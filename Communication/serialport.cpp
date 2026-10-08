#include "serialport.h"
#include <QSerialPortInfo>

SerialPort::SerialPort(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
{
    connect(
        m_serialPort,
        &QSerialPort::readyRead,
        this,
        &SerialPort::readData
        );

    connect(
        m_serialPort,
        &QSerialPort::errorOccurred,
        this,
        [this](QSerialPort::SerialPortError error)
        {
            if (error == QSerialPort::NoError)
                return;

            QString message =
                m_serialPort->errorString();

            emit errorOccurred(message);

            // 单独处理设备拔出这种情况
            if (error == QSerialPort::ResourceError)
            {
                close();
            }
        }
        );
}

// 打开串口
// 9600 8N1
bool SerialPort::open(
    const QString &portName,
    qint32 baudRate,
    QSerialPort::DataBits dataBits,
    QSerialPort::Parity parity,
    QSerialPort::StopBits stopBits)
{
    if (m_serialPort->isOpen())
        m_serialPort->close();

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(dataBits);
    m_serialPort->setParity(parity);
    m_serialPort->setStopBits(stopBits);
    m_serialPort->setFlowControl(
        QSerialPort::NoFlowControl
        );

    if (!m_serialPort->open(QIODevice::ReadWrite))
        return false;

    m_buffer.clear();
    emit opened();

    return true;
}

// 关闭串口
void SerialPort::close()
{
    if (!m_serialPort->isOpen())
        return;

    m_serialPort->close();

    m_buffer.clear();

    emit closed();
}

// 状态查询
bool SerialPort::isOpen() const
{
    return m_serialPort->isOpen();
}

// 发送
bool SerialPort::sendData(const QByteArray &data)
{
    if (!m_serialPort->isOpen())
        return false;

    return m_serialPort->write(data) != -1;
}

// 接收
void SerialPort::readData()
{
    QByteArray data =
        m_serialPort->readAll();

    if(data.isEmpty())
        return;


    emit rawDataReceived(data);


    if(!m_modbusMode)
    {
        m_buffer.append(data);
        processBuffer();
    }
}

// 解析
void SerialPort::processBuffer(){
    while (true)
    {
        // 1. 查找帧头 AA
        int headerIndex =
            m_buffer.indexOf(char(0xAA));

        // 没找到帧头
        if (headerIndex == -1)
        {
            m_buffer.clear();
            return;
        }

        // 丢弃帧头之前的无效数据
        if (headerIndex > 0)
        {
            m_buffer.remove(0, headerIndex);
        }

        // 2. 数据不足一整帧
        if (m_buffer.size() < 7)
        {
            return;
        }

        // 3. 检查帧尾 55
        if (static_cast<unsigned char>(m_buffer[6]) != 0x55)
        {
            // 当前 AA 不是一个合法帧
            // 丢掉这个 AA，继续寻找下一个
            m_buffer.remove(0, 1);
            continue;
        }

        // 4. 提取完整帧
        QByteArray frame = m_buffer.left(7);

        // 5. 从缓冲区删除已经处理的帧
        m_buffer.remove(0, 7);

        // 6. 交给上层
        emit dataReceived(frame);
    }
}

// 可用串口
QStringList SerialPort::availablePorts()
{
    QStringList ports;

    const auto portInfos =
        QSerialPortInfo::availablePorts();

    for (const QSerialPortInfo &info : portInfos)
    {
        ports.append(info.portName());
    }

    return ports;
}

void SerialPort::setModbusMode(bool enable)
{
    m_modbusMode=enable;
}

QString SerialPort::errorString() const
{
    return m_serialPort->errorString();
}
