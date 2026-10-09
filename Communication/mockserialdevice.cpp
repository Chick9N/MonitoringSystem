#include "mockserialdevice.h"
#include "../Utils/simulatedata.h"

#include <QDebug>

MockSerialDevice::MockSerialDevice(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_dataTimer(new QTimer(this))
    , m_temperature(250)   // 25.0℃
    , m_voltage(220)
    , m_online(1)
{
    // 串口收到数据
    connect(
        m_serialPort,
        &QSerialPort::readyRead,
        this,
        &MockSerialDevice::readData
        );

    // 串口错误
    connect(
        m_serialPort,
        &QSerialPort::errorOccurred,
        this,
        [this](QSerialPort::SerialPortError error)
        {
            if (error == QSerialPort::NoError)
                return;

            qWarning()
                << "MockSerialDevice串口错误:"
                << m_serialPort->errorString();
        }
        );

    // 模拟设备数据定时变化
    m_dataTimer->setInterval(1000);

    connect(
        m_dataTimer,
        &QTimer::timeout,
        this,
        &MockSerialDevice::updateMockData
        );
}

MockSerialDevice::~MockSerialDevice()
{
    stop();
}

bool MockSerialDevice::start(
    const QString &portName,
    qint32 baudRate)
{
    if (m_serialPort->isOpen())
    {
        qWarning()
        << "MockSerialDevice已经运行";

        return false;
    }

    m_serialPort->setPortName(portName);

    m_serialPort->setBaudRate(baudRate);

    // 8N1
    m_serialPort->setDataBits(
        QSerialPort::Data8
        );

    m_serialPort->setStopBits(
        QSerialPort::OneStop
        );

    m_serialPort->setParity(
        QSerialPort::NoParity
        );

    m_serialPort->setFlowControl(
        QSerialPort::NoFlowControl
        );

    if (!m_serialPort->open(
            QIODevice::ReadWrite))
    {
        qWarning()
        << "MockSerialDevice打开串口失败:"
        << portName
        << m_serialPort->errorString();

        return false;
    }

    qDebug()
        << "MockSerialDevice已启动:"
        << portName
        << "波特率:"
        << baudRate;

    m_online = 1;

    m_dataTimer->start();

    emit started();

    return true;
}

void MockSerialDevice::stop()
{
    m_dataTimer->stop();

    if (m_serialPort->isOpen())
    {
        m_serialPort->close();
    }

    m_buffer.clear();

    m_online = 0;

    emit stopped();

    qDebug()
        << "MockSerialDevice已停止";
}

bool MockSerialDevice::isRunning() const
{
    return m_serialPort->isOpen();
}

QString MockSerialDevice::portName() const
{
    return m_serialPort->portName();
}

qint32 MockSerialDevice::baudRate() const
{
    return m_serialPort->baudRate();
}

void MockSerialDevice::readData()
{
    QByteArray data = m_serialPort->readAll();

    if (data.isEmpty())
        return;

    // 串口是字节流，先进入缓存
    m_buffer.append(data);

    qDebug()
        << "MockSerialDevice收到数据:"
        << data.toHex(' ').toUpper();

    // 自定义串口请求帧：
    //
    // AA 设备ID 命令 55
    //
    // 例如：
    // AA 01 01 55
    //
    // 共4字节

    while (true)
    {
        // 1. 查找帧头 AA
        int headerIndex =
            m_buffer.indexOf(char(0xAA));

        if (headerIndex == -1)
        {
            m_buffer.clear();
            return;
        }

        // 2. 丢弃帧头之前的无效数据
        if (headerIndex > 0)
        {
            m_buffer.remove(0, headerIndex);
        }

        // 3. 数据不足4字节
        if (m_buffer.size() < 4)
        {
            return;
        }

        // 4. 检查帧尾55
        if (static_cast<unsigned char>(m_buffer[3]) != 0x55)
        {
            // 当前AA不是合法帧
            // 丢掉当前AA，继续寻找下一个
            m_buffer.remove(0, 1);
            continue;
        }

        // 5. 提取完整请求帧
        QByteArray request =
            m_buffer.left(4);

        // 6. 删除已经处理的数据
        m_buffer.remove(0, 4);

        // 7. 处理请求
        processRequest(request);
    }
}

void MockSerialDevice::processRequest(const QByteArray &request)
{
    if (request.size() != 4)
        return;

    // 帧头
    if (static_cast<unsigned char>(request[0]) != 0xAA)
        return;

    // 命令
    if (static_cast<unsigned char>(request[2]) != 0x01)
        return;

    // 帧尾
    if (static_cast<unsigned char>(request[3]) != 0x55)
        return;

    quint8 deviceId =
        static_cast<quint8>(request[1]);

    qDebug() << "MockSerialDevice收到读取请求:"
             << request.toHex(' ').toUpper()
             << "设备ID:" << deviceId;

    // 生成当前模拟数据
    quint8 temperature =
        static_cast<quint8>(m_temperature / 10);

    quint8 voltageHigh =
        static_cast<quint8>((m_voltage >> 8) & 0xFF);

    quint8 voltageLow =
        static_cast<quint8>(m_voltage & 0xFF);

    quint8 online =
        static_cast<quint8>(m_online);

    // 构造7字节响应
    QByteArray response;

    response.append(char(0xAA));
    response.append(char(deviceId));
    response.append(char(temperature));
    response.append(char(voltageHigh));
    response.append(char(voltageLow));
    response.append(char(online));
    response.append(char(0x55));

    sendResponse(response);
}

void MockSerialDevice::sendResponse(const QByteArray &response)
{
    if (!m_serialPort->isOpen())
        return;

    qint64 written = m_serialPort->write(response);

    if (written != response.size())
    {
        qWarning() << "MockSerialDevice发送响应失败:"
                   << m_serialPort->errorString();
        return;
    }

    m_serialPort->flush();

    qDebug() << "MockSerialDevice发送响应:"
             << response.toHex(' ').toUpper();
}

void MockSerialDevice::updateMockData()
{
    m_temperature =
        SimulationData::temperature(
            m_temperature
            );

    m_voltage =
        SimulationData::voltage(
            m_voltage
            );

    m_online =
        SimulationData::online();

    qDebug()
        << "MockSerialDevice模拟数据:"
        << "温度" << m_temperature / 10.0
        << "℃"
        << "电压" << m_voltage
        << "V"
        << "在线" << m_online;
}
