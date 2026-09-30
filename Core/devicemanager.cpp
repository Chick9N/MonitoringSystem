#include "devicemanager.h"
#include "../Communication/protocolparser.h"
#include "../Communication/modbusrtu.h"

#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>

DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_alarmManager(new AlarmManager(this))
    , m_serialPort(new SerialPort(this))
    , m_modbusTCP(new ModbusTCP(this))
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
            // 当前不是自定义协议时，不进行解析
            if (m_protocolType != ProtocolType::Custom)
                return;

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

    m_timeoutTimer = new QTimer(this);

    connect(
        m_timeoutTimer,
        &QTimer::timeout,
        this,
        &DeviceManager::checkDeviceTimeout
        );

    m_timeoutTimer->start(1000);

    // Modbus
    // 接收 Modbus RTU 原始数据
    connect(
        m_serialPort,
        &SerialPort::rawDataReceived,
        this,
        &DeviceManager::handleModbusRawData
        );

    connect(
        m_modbusTCP,
        &ModbusTCP::rawDataReceived,
        this,
        &DeviceManager::handleModbusTCPResponse
        );

    connect(
        m_modbusTCP,
        &ModbusTCP::dataReceived,
        this,
        &DeviceManager::handleModbusTCPResponse
        );

    // Modbus 请求超时定时器
    m_modbusTimeoutTimer = new QTimer(this);
    m_modbusTimeoutTimer->setSingleShot(true);

    connect(
        m_modbusTimeoutTimer,
        &QTimer::timeout,
        this,
        [this]()
        {
            if (!m_modbusRequestPending)
                return;

            qWarning() << "Modbus RTU 请求超时:"
                       << "从站地址:" << m_expectedDeviceId;

            // 结束当前请求
            m_modbusRequestPending = false;

            // 清理未完成的响应数据
            m_modbusBuffer.clear();
        }
        );

    // 轮询
    m_modbusPollTimer = new QTimer(this);

    // Modbus 动态轮询
    m_modbusPollTimer = new QTimer(this);

    connect(m_modbusPollTimer,
            &QTimer::timeout,
            this,
            &DeviceManager::pollNextDevice);

    m_modbusPollTimer->start(1000);

    // 服务器
    m_modbusTCP->connectToDevice("127.0.0.1", 1502);

    // 测试
}

void DeviceManager::addDevice(Device *device)
{
    if (!device)
        return;

    if (getDevice(device->id()))
        return;

    m_devices.append(device);

    connect(device,
            &Device::dataUpdated,
            this,
            [this, device](const DeviceData &data) {
                qDebug() << "DeviceManager 转发数据:"
                         << device->id()
                         << data.temperature
                         << data.voltage
                         << data.isOnline;

                emit deviceDataUpdated(device->id(), data);
            });

    refreshPollDeviceIds();

    emit deviceAdded(device->id());
}

void DeviceManager::removeDevice(int deviceId)
{
    Device* device = getDevice(deviceId);

    if (!device) {
        return;
    }

    // 如果当前正在等待该设备的响应，取消请求
    if (m_modbusRequestPending &&
        m_expectedDeviceId == static_cast<quint8>(deviceId)) {
        m_modbusTimeoutTimer->stop();
        m_modbusRequestPending = false;
        m_modbusBuffer.clear();
    }

    // 从设备列表中移除
    m_devices.removeOne(device);

    // 清理设备接收时间
    m_lastReceivedTime.remove(deviceId);

    // 断开设备信号连接
    disconnect(device, nullptr, this, nullptr);

    // 刷新轮询列表
    refreshPollDeviceIds();

    // 延迟销毁设备
    device->deleteLater();

    qDebug() << "设备已删除:" << deviceId;
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

void DeviceManager::setProtocolType(
    ProtocolType type)
{
    m_protocolType=type;


    if(type == ProtocolType::ModbusRTU)
    {
        m_serialPort->setModbusMode(true);
    }
    else
    {
        m_serialPort->setModbusMode(false);
    }
}

ProtocolType DeviceManager::protocolType() const
{
    return m_protocolType;
}

bool DeviceManager::requestModbusRead(
    quint8 slaveAddress,
    quint16 startAddress,
    quint16 quantity)
{
    // 确认当前使用 Modbus RTU
    if (m_protocolType != ProtocolType::ModbusRTU)
        return false;

    // 确认当前为串口数据源
    if (m_dataSource != DataSource::Serial)
        return false;

    // 确认串口已打开
    if (!m_modbusSimulationMode && !m_serialPort->isOpen())
        return false;

    // 当前请求尚未完成，不允许重复发送
    if (m_modbusRequestPending)
        return false;

    // 构造 Modbus RTU 请求帧
    QByteArray request =
        ModbusRTU::buildReadHoldingRegistersRequest(
            slaveAddress,
            startAddress,
            quantity
            );

    if (request.isEmpty())
        return false;

    // 记录本次请求的上下文
    m_expectedDeviceId = slaveAddress;
    m_expectedQuantity = quantity;
    m_modbusBuffer.clear();
    m_modbusRequestPending = true;

    // 发送请求
    if (!m_modbusSimulationMode) {
        if (!m_serialPort->sendData(request)) {
            m_modbusRequestPending = false;
            m_modbusBuffer.clear();
            return false;
        }
    } else {
        qDebug() << "模拟 Modbus RTU 请求:"
                 << request.toHex(' ');

        // 延迟模拟从站响应，模拟通信过程
        QTimer::singleShot(100, this,
                           [this, slaveAddress, quantity]() {
                               if (!m_modbusSimulationMode ||
                                   !m_modbusRequestPending ||
                                   m_expectedDeviceId != slaveAddress) {
                                   return;
                               }

                               QByteArray response =
                                   buildSimulatedModbusResponse(
                                       slaveAddress,
                                       quantity
                                       );

                               if (response.isEmpty())
                                   return;

                               qDebug() << "模拟从站响应:"
                                        << response.toHex(' ');

                               simulateModbusResponse(response);
                           });
    }

    // 启动请求超时计时
    m_modbusTimeoutTimer->start(ModbusTimeoutMs);

    qDebug() << "发送 Modbus RTU 请求:"
             << request.toHex(' ');

    return true;
}

void DeviceManager::processModbusRegisters(
    quint8 slaveAddress,
    const QVector<quint16> &registers)
{
    if (!getDevice(slaveAddress)) {
        qWarning() << "忽略已移除设备的 Modbus 响应:"
                   << slaveAddress;
        return;
    }

    if (registers.size() < 3) {
        qWarning() << "Modbus register count insufficient:"
                   << registers.size();
        return;
    }

    DeviceData data;
    data.temperature = registers[0] / 10.0;
    data.voltage = registers[1];
    data.isOnline = (registers[2] == 1);

    m_lastReceivedTime[slaveAddress] = QDateTime::currentDateTime();

    updateDeviceData(slaveAddress, data);
}

void DeviceManager::refreshPollDeviceIds()
{
    // 尽量保留当前正在等待轮询的设备
    int currentDeviceId = -1;

    if (!m_pollDeviceIds.isEmpty() &&
        m_currentPollIndex < m_pollDeviceIds.size()) {
        currentDeviceId = m_pollDeviceIds[m_currentPollIndex];
    }

    // 从当前设备容器重新获取 ID
    m_pollDeviceIds.clear();

    for (Device *device : m_devices) {
        if (device)
            m_pollDeviceIds.append(device->id());
    }

    // 排序，保证轮询顺序稳定
    std::sort(m_pollDeviceIds.begin(),m_pollDeviceIds.end());

    if (m_pollDeviceIds.isEmpty()) {
        m_currentPollIndex = 0;
        return;
    }

    // 如果原来的设备仍存在，继续从它开始
    int newIndex = m_pollDeviceIds.indexOf(currentDeviceId);

    if (newIndex >= 0) {
        m_currentPollIndex = newIndex;
    } else {
        // 原设备已删除，确保索引有效
        m_currentPollIndex %= m_pollDeviceIds.size();
    }
}

void DeviceManager::pollNextDevice()
{
    if (m_protocolType != ProtocolType::ModbusRTU &&
        m_protocolType != ProtocolType::ModbusTCP)
    {
        return;
    }


    if (m_dataSource != DataSource::Serial &&
        !m_modbusSimulationMode)
    {
        return;
    }


    // 上一次请求未完成
    if (m_modbusRequestPending)
    {
        return;
    }


    // RTU需要检查串口
    if (m_protocolType == ProtocolType::ModbusRTU)
    {
        if (!m_modbusSimulationMode &&
            !m_serialPort->isOpen())
        {
            return;
        }
    }


    refreshPollDeviceIds();


    if (m_pollDeviceIds.isEmpty())
        return;



    int deviceId =
        m_pollDeviceIds[m_currentPollIndex];


    m_currentPollIndex =
        (m_currentPollIndex + 1)
        % m_pollDeviceIds.size();



    if (!getDevice(deviceId))
        return;



    bool success = false;



    switch (m_protocolType)
    {

    case ProtocolType::ModbusRTU:

        success = requestModbusRead(
            static_cast<quint8>(deviceId),
            0,
            3
            );

        break;



    case ProtocolType::ModbusTCP:

        success = requestModbusTCPRead(
            static_cast<quint8>(deviceId),
            0,
            3
            );

        break;



    default:
        break;
    }



    if (!success)
    {
        qWarning()
        << "Modbus轮询请求发送失败:"
        << deviceId;
    }
}

void DeviceManager::handleModbusRawData(
    const QByteArray &data)
{

    switch(m_protocolType)
    {

    case ProtocolType::ModbusRTU:

        handleModbusRTUResponse(data);
        break;


    case ProtocolType::ModbusTCP:

        handleModbusTCPResponse(data);
        break;


    default:

        break;
    }

}

void DeviceManager::handleModbusRTUResponse(
    const QByteArray &data)
{

    if(!m_modbusRequestPending)
        return;


    m_modbusBuffer.append(data);


    QByteArray frame;


    if(!ModbusRTU::tryExtractResponseFrame(
            m_modbusBuffer,
            m_expectedDeviceId,
            m_expectedQuantity,
            frame))
    {
        return;
    }



    QVector<quint16> registers;

    QString error;



    bool success =
        ModbusRTU::parseReadHoldingRegistersResponse(
            frame,
            m_expectedDeviceId,
            m_expectedQuantity,
            registers,
            error
            );



    m_modbusTimeoutTimer->stop();

    m_modbusRequestPending=false;



    if(!success)
    {
        qWarning()
        <<"Modbus RTU响应失败:"
        <<error;

        return;
    }



    qDebug()
        <<"Modbus RTU响应成功:"
        <<registers;



    processModbusRegisters(
        m_expectedDeviceId,
        registers
        );
}

void DeviceManager::handleModbusTCPResponse(
    const QByteArray &data)
{
    if(!m_modbusRequestPending)
        return;


    QVector<quint16> registers;

    QString error;


    bool ok =
        ModbusTCP::parseReadHoldingRegistersResponse(
            data,
            m_expectedUnitId,
            m_expectedTransactionId,
            m_expectedQuantity,
            registers,
            error
            );


    m_modbusRequestPending=false;


    if(!ok)
    {
        qWarning()
        <<"Modbus TCP解析失败:"
        <<error;

        return;
    }


    qDebug()
        <<"Modbus TCP响应成功:"
        <<registers;


    processModbusRegisters(
        m_expectedUnitId,
        registers
        );
}

bool DeviceManager::connectModbusTCP(
    const QString &ip,
    quint16 port)
{
    m_modbusTCP->connectToDevice(
        ip,
        port
        );

    return true;
}

bool DeviceManager::requestModbusTCPRead(
    quint8 unitId,
    quint16 startAddress,
    quint16 quantity)
{

    static quint16 transactionId = 0;

    transactionId++;


    // 保存当前请求上下文
    m_expectedTransactionId = transactionId;
    m_expectedUnitId = unitId;
    m_expectedQuantity = quantity;

    m_modbusRequestPending = true;

    bool success =
        m_modbusTCP->sendReadHoldingRegistersRequest(
            transactionId,
            unitId,
            startAddress,
            quantity
            );


    if(!success)
    {
        m_modbusRequestPending=false;
    }


    return success;
}

// 测试入口相关
void DeviceManager::simulateSerialData(const QByteArray &data)
{
    m_serialPort->simulateReceive(data);
}

void DeviceManager::simulateModbusResponse(const QByteArray &data)
{
    if (!m_modbusSimulationMode) {
        qWarning() << "当前未开启 Modbus 模拟模式";
        return;
    }

    if (!m_modbusRequestPending) {
        qWarning() << "当前没有待处理的 Modbus 请求";
        return;
    }

    handleModbusRawData(data);
}

QByteArray DeviceManager::buildSimulatedModbusResponse(
    quint8 slaveAddress,
    quint16 quantity)
{
    if (quantity == 0 || quantity > 125)
        return {};

    // 模拟寄存器数据
    QVector<quint16> registers;

    // 不同从站使用不同的测试数据
    switch (slaveAddress) {
    case 1:
        registers = {256, 220, 1};  // 25.6℃、220V、在线
        break;

    case 2:
        registers = {315, 225, 1};  // 31.5℃、225V、在线
        break;

    case 3:
        registers = {280, 215, 1};  // 28.0℃、215V、在线
        break;

    default:
        registers = {300, 220, 1};
        break;
    }

    // 补足请求的寄存器数量
    while (registers.size() < quantity)
        registers.append(0);

    // 生成响应帧
    QByteArray response;
    response.append(static_cast<char>(slaveAddress));
    response.append(static_cast<char>(0x03));
    response.append(static_cast<char>(quantity * 2));

    for (int i = 0; i < quantity; ++i) {
        response.append(
            static_cast<char>((registers[i] >> 8) & 0xFF)
            );
        response.append(
            static_cast<char>(registers[i] & 0xFF)
            );
    }

    // 添加 Modbus CRC，低字节在前
    quint16 crc = ModbusRTU::calculateCRC(response);

    response.append(static_cast<char>(crc & 0xFF));
    response.append(static_cast<char>((crc >> 8) & 0xFF));

    return response;
}

void DeviceManager::setModbusSimulationMode(bool enabled)
{
    m_modbusSimulationMode = enabled;

    if (enabled) {
        m_dataSource = DataSource::Serial;
    }

    qDebug() << "Modbus 模拟模式:"
             << (enabled ? "开启" : "关闭");
}

void DeviceManager::setAutoModbusResponse(bool enabled)
{
    m_autoModbusResponse = enabled;
}
