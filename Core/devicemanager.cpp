#include "devicemanager.h"
#include "../Communication/protocolparser.h"

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
}

void DeviceManager::addDevice(Device *device){
    if(!device)
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

    if (!device)
        return;

    device->setData(data);

    m_alarmManager->checkDeviceData(
        deviceId,
        data
        );

    qDebug() << "收到设备数据:"
             << deviceId
             << data.temperature
             << data.voltage
             << data.isOnline;
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

bool DeviceManager::openSerialPort(
    const QString &portName,
    qint32 baudRate)
{
    return m_serialPort->open(portName, baudRate);
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

