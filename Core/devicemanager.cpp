#include "devicemanager.h"
#include "../Communication/protocolparser.h"
#include "../Communication/modbusrtu.h"

#include <QTimer>
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>

DeviceManager::DeviceManager(DatabaseManager *databaseManager,
QObject *parent)
    : QObject(parent)
    , m_alarmManager(new AlarmManager(this))
    , m_serialPort(new SerialPort(this))
    , m_modbusThread(new QThread(this))
    , m_tcpTimeoutTimer(new QTimer(this))
    , m_modbusTCP(new ModbusTCP())
    , m_databaseManager(databaseManager)
// 这里不能给 ModbusTCP 设置 DeviceManager 为 parent，
// 否则 QObject 不允许将有父对象的实例移动到其他线程。
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

    // Modbus 动态轮询
    m_modbusPollTimer = new QTimer(this);

    connect(m_modbusPollTimer,
            &QTimer::timeout,
            this,
            &DeviceManager::pollNextDevice);

    // 初始化设备列表完成后由 MainWindow 启动轮询。

    // 服务器
    // 多线程
    // 将通信对象移入工作线程
    m_modbusTCP->moveToThread(m_modbusThread);


    // 连接请求信号，使用队列连接
    connect(this, &DeviceManager::connectTCPRequested,
            m_modbusTCP, &ModbusTCP::connectToDevice,
            Qt::QueuedConnection);

    // 接收完整响应帧，返回主线程处理
    connect(m_modbusTCP, &ModbusTCP::dataReceived,
            this, &DeviceManager::handleModbusTCPResponse,
            Qt::QueuedConnection);

    connect(m_modbusThread, &QThread::finished,
            m_modbusTCP, &QObject::deleteLater);

    connect(this, &DeviceManager::sendTCPReadRequest,
            m_modbusTCP, &ModbusTCP::sendReadHoldingRegistersRequest,
            Qt::QueuedConnection);

    // 工作线程将实际发送结果通知主线程，主线程再更新请求状态。
    connect(m_modbusTCP, &ModbusTCP::sendResult,
            this, [this](quint16 transactionId, bool success)
            {
                // 忽略非当前请求的发送结果
                if (!m_tcpRequestPending ||
                    transactionId != m_tcpExpectedTransactionId) {
                    return;
                }

                if (!success)
                {
                    // 发送失败时同步停止计时器
                    m_tcpTimeoutTimer->stop();
                    m_tcpRequestPending  = false;
                    qDebug() << "Modbus TCP 请求发送失败:"
                             << transactionId;
                }
            }, Qt::QueuedConnection);

    connect(
        m_modbusTCP,
        &ModbusTCP::tcpConnected,
        this,
        [this]()
        {
            m_tcpConnected = true;

            qDebug()
                << "主线程：Modbus TCP已连接";


            // 停止可能存在的TCP超时状态
            m_tcpTimeoutTimer->stop();
            m_tcpRequestPending = false;


            // 不要这里直接设置设备在线
            // 等下一轮Modbus轮询成功后恢复
        },
        Qt::QueuedConnection
        );

    connect(
        m_modbusTCP,
        &ModbusTCP::tcpDisconnected,
        this,
        [this]()
        {
            qDebug()
            <<"Modbus TCP连接断开";


            m_tcpConnected=false;


            m_tcpTimeoutTimer->stop();
            m_tcpRequestPending=false;


            // TCP断开，所有TCP设备离线

            for(Device *device:m_devices)
            {
                if(device->config().protocolType
                    != ProtocolType::ModbusTCP)
                    continue;


                DeviceData data=device->data();

                data.isOnline=false;

                device->setData(data);
            }

        });

    connect(m_modbusTCP, &ModbusTCP::tcpError,
            this, [](const QString &error) {
                qWarning() << "主线程：Modbus TCP错误:" << error;
            }, Qt::QueuedConnection);

    m_tcpTimeoutTimer->setSingleShot(true);

    connect(m_tcpTimeoutTimer, &QTimer::timeout,
            this, [this]() {
                if (!m_tcpRequestPending)
                    return;

                qWarning() << "Modbus TCP请求超时:"
                           << m_tcpExpectedTransactionId;

                m_tcpRequestPending = false;
            });

    connect(
        m_modbusTCP,
        &ModbusTCP::requestTimeout,
        this,
        &DeviceManager::onModbusTimeout
        );

    // tcp连接测试
    connect(
        m_modbusTCP,
        &ModbusTCP::tcpConnected,
        this,
        [this]()
        {
            emit tcpTestResult(
                true,
                "Modbus TCP连接成功"
                );
        }
        );


    connect(
        m_modbusTCP,
        &ModbusTCP::tcpError,
        this,
        [this](const QString &error)
        {
            emit tcpTestResult(
                false,
                error
                );
        }
        );


    // 启动工作线程
    m_modbusThread->start();
    //emit connectModbusTCP("127.0.0.1", 1502);

    // 注意：此处不主动连接 Modbus TCP。
    // 应在外部 Modbus TCP Server 成功监听端口后，
    // 由初始化流程调用 connectModbusTCP(ip, port)。
}

DeviceManager::~DeviceManager()
{
    if (m_modbusThread && m_modbusThread->isRunning()) {
        // 在工作线程中关闭 socket
        QMetaObject::invokeMethod(
            m_modbusTCP,
            "shutdown",
            Qt::BlockingQueuedConnection
            );

        // 结束线程事件循环
        m_modbusThread->quit();

        // 等待线程真正结束
        m_modbusThread->wait();
    }
}

bool DeviceManager::addDevice(Device *device)
{
    if (!device)
        return false;

    if (getDevice(device->id()))
        return false;

    DeviceConfig config = device->config();

    // 串口设备才需要初始化串口
    if (config.dataSource == DataSource::Serial)
    {
        if (!openSerialForDevice(config))
        {
            qWarning()
            << "设备添加失败，串口初始化失败:"
            << config.deviceId;

            device->deleteLater();
            return false;
        }
    }

    m_devices.append(device);

    connect(
        device,
        &Device::dataUpdated,
        this,
        [this, device](const DeviceData &data)
        {
            qDebug()
            << "DeviceManager 转发数据:"
            << device->id()
            << data.temperature
            << data.voltage
            << data.isOnline;

            emit deviceDataUpdated(
                device->id(),
                data
                );
        }
        );

    refreshPollDeviceIds();

    emit deviceAdded(device->id());

    return true;
}

void DeviceManager::removeDevice(int deviceId)
{
    Device* device = getDevice(deviceId);

    if (!device) {
        return;
    }

    // 删除数据库中的设备配置
    if (m_databaseManager &&
        !m_databaseManager->deleteDevice(deviceId))
    {
        qWarning() << "数据库删除设备失败，取消删除:"
                   << deviceId;
        return;
    }

    // 如果当前正在等待该设备的RTU响应，取消请求
    if (m_modbusRequestPending &&
        m_expectedDeviceId == static_cast<quint8>(deviceId)) {
        m_modbusTimeoutTimer->stop();
        m_modbusRequestPending = false;
        m_modbusBuffer.clear();
    }

    // 如果当前正在等待该设备的TCP响应，取消请求
    if (m_tcpRequestPending &&
        m_tcpExpectedApplicationDeviceId == deviceId) {
        m_tcpTimeoutTimer->stop();
        m_tcpRequestPending = false;
        m_tcpExpectedApplicationDeviceId = -1;
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

    emit deviceRemoved(deviceId);

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

void DeviceManager::updateAllDevices()
{
    for (Device* device : m_devices) {
        if (device->config().dataSource != DataSource::Simulation)
            continue;

        device->updateData();

        m_alarmManager->checkDeviceData(
            device->id(),
            device->data()
            );
    }
}

void DeviceManager::startPolling()
{
    if (m_modbusPollTimer && !m_modbusPollTimer->isActive())
        m_modbusPollTimer->start(1000);
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

bool DeviceManager::openSerialForDevice(const DeviceConfig &config)
{
    qDebug()
    << "DeviceManager准备打开串口:"
    << "port =" << config.serialPort
    << "baud =" << config.baudRate
    << "dataBits =" << config.dataBits
    << "parity =" << config.parity
    << "stopBits =" << config.stopBits;

    if (config.dataSource != DataSource::Serial)
        return true;

    QSerialPort::DataBits dataBits;

    switch (config.dataBits)
    {
    case 5:
        dataBits = QSerialPort::Data5;
        break;

    case 6:
        dataBits = QSerialPort::Data6;
        break;

    case 7:
        dataBits = QSerialPort::Data7;
        break;

    case 8:
    default:
        dataBits = QSerialPort::Data8;
        break;
    }

    QSerialPort::Parity parity;

    if (config.parity == "Even")
        parity = QSerialPort::EvenParity;
    else if (config.parity == "Odd")
        parity = QSerialPort::OddParity;
    else
        parity = QSerialPort::NoParity;

    QSerialPort::StopBits stopBits;

    if (config.stopBits == 2)
        stopBits = QSerialPort::TwoStop;
    else
        stopBits = QSerialPort::OneStop;

    // 如果串口已经打开，暂时认为当前串口就是正在使用的串口
    if (m_serialPort->isOpen())
    {
        return true;
    }

    if (!openSerialPort(
            config.serialPort,
            config.baudRate,
            dataBits,
            parity,
            stopBits))
    {
        qWarning() << "打开串口失败:"
                   << config.serialPort
                   << m_serialPort->errorString();

        return false;
    }

    m_protocolType = config.protocolType;

    m_serialPort->setModbusMode(
        config.protocolType == ProtocolType::ModbusRTU
        );

    qDebug() << "DeviceManager串口初始化成功:"
             << config.serialPort
             << "波特率:" << config.baudRate
             << "协议:" << static_cast<int>(config.protocolType);

    return true;
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

void DeviceManager::checkDeviceTimeout()
{
    const QDateTime now = QDateTime::currentDateTime();

    for (Device *device : m_devices)
    {
        if (!device)
            continue;

        if (device->config().dataSource != DataSource::Serial)
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

bool DeviceManager::requestModbusRead(
    quint8 slaveAddress,
    quint16 startAddress,
    quint16 quantity,
    int applicationDeviceId
    )
{
    // 确认串口已打开
    if (!m_serialPort->isOpen())
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

    // 记录本次请求上下文
    m_expectedDeviceId = slaveAddress;
    m_expectedQuantity = quantity;
    m_expectedApplicationDeviceId = applicationDeviceId;
    m_modbusBuffer.clear();
    m_modbusRequestPending = true;

    // 发送请求
    if (!m_serialPort->sendData(request)) {
        m_modbusRequestPending = false;
        m_modbusBuffer.clear();
        return false;
    }

    // 启动请求超时计时
    m_modbusTimeoutTimer->start(ModbusTimeoutMs);

    qDebug() << "发送 Modbus RTU 请求:"
             << request.toHex(' ');

    return true;
}

void DeviceManager::processModbusRegisters(
    int applicationDeviceId,
    const QVector<quint16> &registers)
{
    if (!getDevice(applicationDeviceId)) {
        qWarning() << "忽略已移除设备的 Modbus 响应:"
                   << applicationDeviceId;
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

    m_lastReceivedTime[applicationDeviceId] =
        QDateTime::currentDateTime();

    updateDeviceData(applicationDeviceId, data);
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
    refreshPollDeviceIds();

    if (m_pollDeviceIds.isEmpty())
        return;

    // 当前全局只允许一个 RTU 或 TCP 请求待处理
    if (m_modbusRequestPending || m_tcpRequestPending)
        return;

    // 按轮询索引取得设备
    int deviceId = m_pollDeviceIds[m_currentPollIndex];

    m_currentPollIndex =
        (m_currentPollIndex + 1) % m_pollDeviceIds.size();

    Device *device = getDevice(deviceId);
    if (!device)
        return;

    const DeviceConfig &config = device->config();

    bool success = false;

    switch (config.protocolType)
    {
    case ProtocolType::ModbusRTU:
    {
        // RTU 使用串口或模拟数据源
        if (config.dataSource == DataSource::Serial)
        {
            if (!m_serialPort->isOpen())
                return;
        }
        else if (config.dataSource == DataSource::Simulation)
        {
            return;
        }
        else
        {
            return;
        }

        if (config.rtuSlaveId < 1 || config.rtuSlaveId > 247 ||
            config.rtuStartAddress < 0 ||
            config.rtuStartAddress > 65535 ||
            config.rtuQuantity < 1 ||
            config.rtuQuantity > 125)
        {
            qWarning() << "RTU设备配置无效:" << deviceId;
            return;
        }

        success = requestModbusRead(
            static_cast<quint8>(config.rtuSlaveId),
            static_cast<quint16>(config.rtuStartAddress),
            static_cast<quint16>(config.rtuQuantity),
            deviceId
            );
        break;
    }

    case ProtocolType::ModbusTCP:
    {
        if (config.dataSource != DataSource::TCP)
            return;

        if (!m_tcpConnected)
            return;

        if (config.tcpUnitId < 0 || config.tcpUnitId > 255 ||
            config.tcpStartAddress < 0 ||
            config.tcpStartAddress > 65535 ||
            config.tcpQuantity < 1 ||
            config.tcpQuantity > 125)
        {
            qWarning() << "TCP设备配置无效:" << deviceId;
            return;
        }

        success = requestModbusTCPRead(
            static_cast<quint8>(config.tcpUnitId),
            static_cast<quint16>(config.tcpStartAddress),
            static_cast<quint16>(config.tcpQuantity),
            deviceId
            );
        break;
    }

    case ProtocolType::Custom:
    {
        if (config.dataSource != DataSource::Serial)
            return;

        if (!m_serialPort->isOpen())
            return;

        QByteArray request;
        request.append(static_cast<char>(0xAA));
        request.append(static_cast<char>(deviceId));
        request.append(static_cast<char>(0x01));
        request.append(static_cast<char>(0x55));

        if (!m_serialPort->sendData(request))
        {
            qWarning() << "自定义串口请求发送失败:"
                       << deviceId;
            return;
        }

        qDebug() << "发送自定义串口请求:"
                 << request.toHex(' ').toUpper()
                 << "设备ID:" << deviceId;

        success = true;
        break;
    }
        return;
    }

    if (!success)
    {
        qWarning() << "Modbus轮询请求发送失败:"
                   << deviceId;
    }
}

void DeviceManager::handleModbusRawData(
    const QByteArray &data)
{
    if (!m_modbusRequestPending)
        return;

    handleModbusRTUResponse(data);
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
        m_expectedApplicationDeviceId,
        registers
        );
}

void DeviceManager::handleModbusTCPResponse(const QByteArray &data)
{
    if (!m_tcpRequestPending)
        return;

    if (data.size() < 2) {
        qWarning() << "Modbus TCP响应过短";
        return;
    }

    quint16 transactionId =
        (static_cast<quint8>(data[0]) << 8) |
        static_cast<quint8>(data[1]);

    // 事务ID不匹配，不结束当前请求
    if (transactionId != m_tcpExpectedTransactionId) {
        qWarning() << "收到非当前请求的响应:" << transactionId;
        return;
    }

    QVector<quint16> registers;
    QString error;

    bool ok = ModbusTCP::parseReadHoldingRegistersResponse(
        data,
        m_tcpExpectedUnitId,
        m_tcpExpectedTransactionId,
        m_tcpExpectedQuantity,
        registers,
        error
        );

    // 收到对应事务的响应后，结束本次等待
    m_tcpTimeoutTimer->stop();
    m_tcpRequestPending = false;

    if (!ok) {
        qWarning() << "Modbus TCP解析失败:" << error;
        return;
    }

    qDebug() << "Modbus TCP响应成功:" << registers;

    processModbusRegisters(m_tcpExpectedApplicationDeviceId, registers);
}

bool DeviceManager::connectModbusTCP(
    const QString &ip,
    quint16 port)
{
    if (!m_modbusThread || !m_modbusThread->isRunning())
        return false;

    emit connectTCPRequested(ip, port);
    return true;
}

bool DeviceManager::requestModbusTCPRead(
    quint8 unitId,
    quint16 startAddress,
    quint16 quantity,
    int applicationDeviceId
    )
{
    if (!m_tcpConnected) {
        qDebug() << "Modbus TCP未连接，跳过读取请求";
        return false;
    }

    if (m_tcpRequestPending)
        return false;

    static quint16 transactionId = 0;
    transactionId++;

    // 保存当前请求上下文
    m_tcpExpectedTransactionId = transactionId;
    m_tcpExpectedUnitId = unitId;
    m_tcpExpectedQuantity = quantity;
    m_tcpExpectedApplicationDeviceId = applicationDeviceId;

    m_tcpRequestPending = true;

    // 按照信号声明顺序发送请求
    emit sendTCPReadRequest(
        transactionId,
        unitId,
        startAddress,
        quantity
        );

    // 启动超时计时器
    m_tcpTimeoutTimer->start(3000);

    return true;
}

void DeviceManager::testTCPConnection(
    const QString &ip,
    quint16 port
    )
{
    qDebug()
    << "测试Modbus TCP连接:"
    << ip
    << port;


    if(!m_modbusTCP)
    {
        qWarning()
        << "ModbusTCP对象为空";

        return;
    }


    emit connectTCPRequested(
        ip,
        port
        );
}

void DeviceManager::onModbusTimeout(quint16 transactionId)
{
    if(transactionId != m_tcpExpectedTransactionId)
        return;


    qWarning()
        <<"TCP设备响应超时:"
        <<m_tcpExpectedApplicationDeviceId;


    Device *device =
        getDevice(m_tcpExpectedApplicationDeviceId);


    if(device)
    {
        DeviceData data=device->data();

        data.isOnline=false;

        device->setData(data);
    }


    m_tcpRequestPending=false;
}
