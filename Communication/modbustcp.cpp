#include "modbustcp.h"
#include <QAbstractSocket>
#include <QDebug>

ModbusTCP::ModbusTCP(QObject *parent)
    : QObject(parent),
    m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, []() {
        qDebug() << "Modbus TCP连接成功";
    });

    connect(m_socket, &QTcpSocket::disconnected, this, []() {
        qDebug() << "Modbus TCP连接断开";
    });

    connect(m_socket, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                qWarning() << "Modbus TCP连接错误:"
                           << m_socket->errorString();
            });

    connect(
        m_socket,
        &QTcpSocket::readyRead,
        this,
        &ModbusTCP::readData
        );
}

QByteArray ModbusTCP::buildReadHoldingRegistersRequest(
    quint16 transactionId,
    quint8 unitId,
    quint16 startAddress,
    quint16 quantity)
{
    QByteArray request;


    // MBAP Header

    // Transaction Identifier
    request.append(
        static_cast<char>((transactionId >> 8) & 0xFF)
        );

    request.append(
        static_cast<char>(transactionId & 0xFF)
        );


    // Protocol Identifier
    // Modbus TCP 固定 0
    request.append(static_cast<char>(0x00));
    request.append(static_cast<char>(0x00));


    // Length
    // 后续长度:
    // UnitId(1)
    // Function(1)
    // Address(2)
    // Quantity(2)
    request.append(static_cast<char>(0x00));
    request.append(static_cast<char>(0x06));


    // Unit Identifier
    request.append(
        static_cast<char>(unitId)
        );


    // Function Code 03
    request.append(0x03);


    // Start Address
    request.append(
        static_cast<char>((startAddress >> 8) & 0xFF)
        );

    request.append(
        static_cast<char>(startAddress & 0xFF)
        );


    // Quantity
    request.append(
        static_cast<char>((quantity >> 8) & 0xFF)
        );

    request.append(
        static_cast<char>(quantity & 0xFF)
        );


    return request;
}

bool ModbusTCP::sendReadHoldingRegistersRequest(
    quint16 transactionId,
    quint8 unitId,
    quint16 startAddress,
    quint16 quantity)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "Modbus TCP尚未连接";
        return false;
    }

    QByteArray request = buildReadHoldingRegistersRequest(
        transactionId,
        unitId,
        startAddress,
        quantity
        );

    qint64 bytesWritten = m_socket->write(request);

    if (bytesWritten != request.size()) {
        qWarning() << "Modbus TCP请求发送失败";
        return false;
    }

    qDebug() << "Modbus TCP发送请求:"
             << request.toHex(' ');

    return true;
}

bool ModbusTCP::parseReadHoldingRegistersResponse(
    const QByteArray &response,
    quint8 expectedUnit,
    quint16 expectedTransaction,
    quint16 expectedQuantity,
    QVector<quint16> &registers,
    QString &error)
{
    registers.clear();
    error.clear();


    // MBAP + PDU 最小长度

    if(response.size() < 9)
    {
        error = "Response too short";
        return false;
    }


    // Transaction ID

    quint16 transaction =
        (static_cast<quint8>(response[0]) << 8)
        |
        static_cast<quint8>(response[1]);


    if(transaction != expectedTransaction)
    {
        error = "Transaction mismatch";
        return false;
    }



    // Protocol ID

    quint16 protocol =
        (static_cast<quint8>(response[2]) << 8)
        |
        static_cast<quint8>(response[3]);


    if(protocol != 0)
    {
        error = "Invalid protocol id";
        return false;
    }



    // Unit ID

    quint8 unit =
        static_cast<quint8>(response[6]);


    if(unit != expectedUnit)
    {
        error = "Unit mismatch";
        return false;
    }



    // 功能码

    quint8 function =
        static_cast<quint8>(response[7]);


    if(function != 0x03)
    {
        error = "Function mismatch";
        return false;
    }



    // 字节数量

    quint8 byteCount =
        static_cast<quint8>(response[8]);


    if(byteCount != expectedQuantity * 2)
    {
        error = "Byte count mismatch";
        return false;
    }



    if(response.size() != 9 + byteCount)
    {
        error = "Length mismatch";
        return false;
    }



    // 数据解析

    for(int i = 0; i < byteCount; i += 2)
    {
        quint16 value =
            (static_cast<quint8>(response[9+i]) << 8)
            |
            static_cast<quint8>(response[10+i]);


        registers.append(value);
    }


    return true;
}

void ModbusTCP::connectToDevice(const QString &ip, quint16 port)
{
    m_socket->connectToHost(ip, port);
}

void ModbusTCP::disconnectFromDevice()
{
    m_socket->disconnectFromHost();
}

void ModbusTCP::readData()
{
    QByteArray data = m_socket->readAll();

    if(data.isEmpty())
        return;


    // TCP字节流进入缓存
    m_buffer.append(data);


    QByteArray frame;


    while(tryExtractFrame(frame))
    {
        emit dataReceived(frame);
    }
}

bool ModbusTCP::sendData(
    const QByteArray &data)
{
    if(m_socket->state()
        != QAbstractSocket::ConnectedState)
    {
        return false;
    }


    return m_socket->write(data)
           != -1;
}

bool ModbusTCP::tryExtractFrame(
    QByteArray &frame)
{
    frame.clear();


    // MBAP最小长度
    if(m_buffer.size() < 6)
        return false;


    // 读取 Length

    quint16 length =
        (static_cast<quint8>(m_buffer[4]) << 8)
        |
        static_cast<quint8>(m_buffer[5]);


    // 完整帧长度

    int frameLength =
        6 + length;


    // 数据还没接收完整

    if(m_buffer.size() < frameLength)
        return false;


    frame =
        m_buffer.left(frameLength);


    m_buffer.remove(
        0,
        frameLength
        );


    return true;
}
