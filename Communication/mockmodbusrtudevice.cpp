#include "mockmodbusrtudevice.h"
#include "../Utils/simulatedata.h"
#include <QDebug>

MockModbusRTUDevice::MockModbusRTUDevice(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_dataTimer(new QTimer(this))
{
    connect(
        m_serialPort,
        &QSerialPort::readyRead,
        this,
        &MockModbusRTUDevice::readData
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

            qWarning()
                << "MockModbusRTUDevice串口错误:"
                << message;

            emit errorOccurred(message);

            if (error == QSerialPort::ResourceError)
            {
                stop();
            }
        }
        );

    connect(
        m_dataTimer,
        &QTimer::timeout,
        this,
        &MockModbusRTUDevice::updateMockData
        );
}

bool MockModbusRTUDevice::start(
    const QString &portName,
    qint32 baudRate)
{
    if (m_serialPort->isOpen())
        m_serialPort->close();

    m_serialPort->setPortName(portName);

    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(
        QSerialPort::NoFlowControl
        );

    if (!m_serialPort->open(QIODevice::ReadWrite))
    {
        emit errorOccurred(
            m_serialPort->errorString()
            );

        return false;
    }

    m_buffer.clear();

    m_dataTimer->start(1000);

    qDebug()
        << "MockModbusRTUDevice已启动:"
        << portName
        << "波特率:"
        << baudRate;

    emit started();

    return true;
}

void MockModbusRTUDevice::stop()
{
    if (!m_serialPort->isOpen())
        return;

    m_dataTimer->stop();

    m_serialPort->close();

    m_buffer.clear();

    qDebug()
        << "MockModbusRTUDevice已停止";

    emit stopped();
}

bool MockModbusRTUDevice::isRunning() const
{
    return m_serialPort->isOpen();
}

void MockModbusRTUDevice::readData()
{
    QByteArray data =
        m_serialPort->readAll();

    if (data.isEmpty())
        return;

    m_buffer.append(data);

    qDebug()
        << "MockModbusRTUDevice收到数据:"
        << data.toHex(' ').toUpper();

    while (true)
    {
        /*
         * Modbus RTU 读保持寄存器请求固定为 8 字节：
         *
         * [从站地址]
         * [功能码]
         * [起始地址高]
         * [起始地址低]
         * [数量高]
         * [数量低]
         * [CRC低]
         * [CRC高]
         */

        if (m_buffer.size() < 8)
            return;

        QByteArray request =
            m_buffer.left(8);

        quint16 receivedCRC =
            static_cast<quint8>(
                static_cast<unsigned char>(
                    request[6]
                    )
                )
            |
            (
                static_cast<quint16>(
                    static_cast<quint8>(
                        static_cast<unsigned char>(
                            request[7]
                            )
                        )
                    )
                << 8
                );

        quint16 calculatedCRC =
            calculateCRC(
                request.left(6)
                );

        if (receivedCRC != calculatedCRC)
        {
            qWarning()
            << "MockModbusRTUDevice请求CRC错误";

            m_buffer.remove(0, 1);
            continue;
        }

        m_buffer.remove(0, 8);

        processRequest(request);
    }
}

void MockModbusRTUDevice::processRequest(
    const QByteArray &request)
{
    if (request.size() != 8)
        return;

    quint8 slaveAddress =
        static_cast<quint8>(
            static_cast<unsigned char>(
                request[0]
                )
            );

    quint8 function =
        static_cast<quint8>(
            static_cast<unsigned char>(
                request[1]
                )
            );

    quint16 startAddress =
        (
            static_cast<quint16>(
                static_cast<quint8>(
                    static_cast<unsigned char>(
                        request[2]
                        )
                    )
                )
            << 8
            )
        |
        static_cast<quint16>(
            static_cast<quint8>(
                static_cast<unsigned char>(
                    request[3]
                    )
                )
            );

    quint16 quantity =
        (
            static_cast<quint16>(
                static_cast<quint8>(
                    static_cast<unsigned char>(
                        request[4]
                        )
                    )
                )
            << 8
            )
        |
        static_cast<quint16>(
            static_cast<quint8>(
                static_cast<unsigned char>(
                    request[5]
                    )
                )
            );

    qDebug()
        << "MockModbusRTUDevice收到读取请求:"
        << "从站:"
        << slaveAddress
        << "功能码:"
        << QString("0x%1")
               .arg(function, 2, 16, QChar('0'))
        << "起始地址:"
        << startAddress
        << "数量:"
        << quantity;

    /*
     * 当前 Mock 只支持：
     *
     * 功能码 03
     * 读取保持寄存器
     */

    if (function != 0x03)
    {
        qWarning()
        << "MockModbusRTUDevice不支持功能码:"
        << function;

        return;
    }

    if (quantity < 1 || quantity > 125)
    {
        qWarning()
        << "MockModbusRTUDevice读取数量非法:"
        << quantity;

        return;
    }

    QByteArray response =
        buildResponse(
            slaveAddress,
            startAddress,
            quantity
            );

    if (response.isEmpty())
        return;

    qDebug()
        << "MockModbusRTUDevice发送响应:"
        << response.toHex(' ').toUpper();

    sendResponse(response);
}

QByteArray MockModbusRTUDevice::buildResponse(
    quint8 slaveAddress,
    quint16 startAddress,
    quint16 quantity)
{
    /*
     * 当前模拟寄存器：
     *
     * 0 → 温度 × 10
     * 1 → 电压
     * 2 → 在线状态
     */

    QVector<quint16> registers;

    for (quint16 i = 0; i < quantity; ++i)
    {
        quint16 address =
            startAddress + i;

        quint16 value = 0;

        switch (address)
        {
        case 0:
            value = m_temperature;
            break;

        case 1:
            value = m_voltage;
            break;

        case 2:
            value = m_online;
            break;

        default:
            /*
             * 未定义寄存器暂时返回 0
             */
            value = 0;
            break;
        }

        registers.append(value);
    }

    QByteArray response;

    response.append(
        static_cast<char>(slaveAddress)
        );

    response.append(
        static_cast<char>(0x03)
        );

    response.append(
        static_cast<char>(quantity * 2)
        );

    for (quint16 value : registers)
    {
        response.append(
            static_cast<char>(
                (value >> 8) & 0xFF
                )
            );

        response.append(
            static_cast<char>(
                value & 0xFF
                )
            );
    }

    quint16 crc =
        calculateCRC(response);

    response.append(
        static_cast<char>(crc & 0xFF)
        );

    response.append(
        static_cast<char>(
            (crc >> 8) & 0xFF
            )
        );

    return response;
}

quint16 MockModbusRTUDevice::calculateCRC(
    const QByteArray &data)
{
    quint16 crc = 0xFFFF;

    for (unsigned char byte : data)
    {
        crc ^= byte;

        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

void MockModbusRTUDevice::sendResponse(
    const QByteArray &response)
{
    if (!m_serialPort->isOpen())
        return;

    qint64 written =
        m_serialPort->write(response);

    if (written == -1)
    {
        qWarning()
        << "MockModbusRTUDevice发送失败:"
        << m_serialPort->errorString();

        return;
    }

    m_serialPort->flush();
}

void MockModbusRTUDevice::updateMockData()
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
        << "MockModbusRTUDevice模拟数据:"
        << "温度:"
        << m_temperature / 10.0
        << "℃"
        << "电压:"
        << m_voltage
        << "V"
        << "在线:"
        << m_online;
}
