#include "devicemanager.h"
#include "../Communication/protocolparser.h"

#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>
DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_alarmManager(new AlarmManager(this))
    , m_serialPort(new SerialPort(this))
{
    // 转发信号
    connect(
        m_alarmManager,
        &AlarmManager::alarmTriggered,
        this,
        &DeviceManager::alarmTriggered
    );

    // 串口
    connect(
        m_serialPort,
        &SerialPort::dataReceived,
        this,
        [this](const QByteArray &frame)
        {
            int deviceId;
            DeviceData data;

            if (!ProtocolParser::parse(
                    frame,
                    deviceId,
                    data))
            {
                return;
            }

            // 记录有效数据帧的接收时间
            m_lastReceivedTime[deviceId] =
                QDateTime::currentDateTime();


            updateDeviceData(deviceId, data);
        }
        );

    connect(
        m_serialPort,
        &SerialPort::opened,
        this,
        &DeviceManager::serialPortOpened
        );

    connect(
        m_serialPort,
        &SerialPort::closed,
        this,
        &DeviceManager::serialPortClosed
        );

    connect(
        m_serialPort,
        &SerialPort::errorOccurred,
        this,
        &DeviceManager::serialPortError
        );

    // 验证检索串口数据流
    qDebug() << m_serialPort->availablePorts();
    qDebug() << m_serialPort->open("COM99");

    m_timeoutTimer = new QTimer(this);

    connect(
        m_timeoutTimer,
        &QTimer::timeout,
        this,
        &DeviceManager::checkDeviceTimeout
        );

    m_timeoutTimer->start(1000);
}

void DeviceManager::addDevice(Device *device){
    if(!device)
        return;

    if (getDevice(device->id()))
        return;

    m_devices.append(device);

    connect(device,
            &Device::dataUpdated,
            this,
            [this,device](const DeviceData &data){
                qDebug() << "DeviceManager 转发数据:"
                         << device->id()
                         << data.temperature
                         << data.voltage
                         << data.isOnline;

                emit deviceDataUpdated(device->id(),data);
            });
}

Device* DeviceManager::getDevice(int deviceId){
    if(deviceId == -1) return nullptr;
    for(Device* device:m_devices){
        if(device->id() == deviceId)
            return device;
    }
    return nullptr;
}

QList<Device*> DeviceManager::devices() const{
    return m_devices;
}

void DeviceManager::updateDeviceData(
    int deviceId,
    const DeviceData &data)
{
    qDebug() << "收到设备数据:"
             << deviceId
             << data.temperature
             << data.voltage
             << data.isOnline;

    Device *device = getDevice(deviceId);

    // 如果设备不存在，则自动创建
    if (!device)
    {
        device = new Device(deviceId, this);
        addDevice(device);

        emit deviceAdded(deviceId);

        qDebug() << "自动发现新设备:" << deviceId;
    }

    // 更新设备数据
    device->setData(data);

    // 检查设备报警
    m_alarmManager->checkDeviceData(
        deviceId,
        data
        );
}

void DeviceManager::updateAllDevices(){
    if (m_dataSource != DataSource::Simulation)
        return;

    for(Device* device:m_devices){
        device->updateData();

        m_alarmManager->checkDeviceData(
            device->id(),
            device->data()
        );
    }
}

void DeviceManager::startAll(){
    for(Device* device:m_devices){
        device->start();
    }
}


void DeviceManager::stopAll(){
    for(Device* device:m_devices){
        device->stop();
    }
}

QStringList DeviceManager::availableSerialPorts() const
{
    return SerialPort::availablePorts();
}

bool DeviceManager::isSerialPortOpen() const
{
    return m_serialPort->isOpen();
}

bool DeviceManager::openSerialPort(
    const QString &portName,
    qint32 baudRate,
    QSerialPort::DataBits dataBits,
    QSerialPort::Parity parity,
    QSerialPort::StopBits stopBits)
{
    return m_serialPort->open(
        portName,
        baudRate,
        dataBits,
        parity,
        stopBits
        );
}

void DeviceManager::closeSerialPort()
{
    m_serialPort->close();
}

void DeviceManager::setDataSource(DataSource source)
{
    m_dataSource = source;
}

DataSource DeviceManager::dataSource() const
{
    return m_dataSource;
}

void DeviceManager::simulateSerialData(const QByteArray &data)
{
    m_serialPort->simulateReceive(data);
}

void DeviceManager::checkDeviceTimeout()
{
    // 仅对串口模式下的设备进行超时检测
    if (m_dataSource != DataSource::Serial)
        return;

    const QDateTime now = QDateTime::currentDateTime();

    for (Device *device : m_devices)
    {
        if (!device)
            continue;

        int deviceId = device->id();

        // 从未收到过串口数据，不做超时判断
        if (!m_lastReceivedTime.contains(deviceId))
            continue;

        // 已经离线，不重复触发
        if (!device->data().isOnline)
            continue;

        qint64 elapsed =
            m_lastReceivedTime.value(deviceId)
                .msecsTo(now);

        if (elapsed > CommunicationTimeoutMs)
        {
            DeviceData data = device->data();
            data.isOnline = false;

            updateDeviceData(deviceId, data);

            qDebug() << "设备通信超时，已标记离线:"
                     << deviceId;
        }
    }
}
