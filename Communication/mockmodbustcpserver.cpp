#include "mockmodbustcpserver.h"
#include <QDebug>
#include <QVector>

MockModbusTCPServer::MockModbusTCPServer(QObject *parent)
    : QObject(parent),
    m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &MockModbusTCPServer::onNewConnection);
}

bool MockModbusTCPServer::start(quint16 port)
{
    if (m_server->isListening()) {
        return true;
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        qWarning() << "Modbus TCP服务器启动失败:"
                   << m_server->errorString();
        return false;
    }

    qDebug() << "Modbus TCP服务器已启动，端口:"
             << m_server->serverPort();

    return true;
}

void MockModbusTCPServer::stop()
{
    if (m_server->isListening()) {
        m_server->close();
        qDebug() << "Modbus TCP服务器已停止";
    }
}

void MockModbusTCPServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();

        qDebug() << "客户端已连接:"
                 << socket->peerAddress().toString()
                 << socket->peerPort();

        m_clientBuffers.insert(socket, QByteArray());

        connect(socket, &QTcpSocket::readyRead,
                this, &MockModbusTCPServer::readRequest);

        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_clientBuffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void MockModbusTCPServer::readRequest()
{
    auto *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    QByteArray &buffer = m_clientBuffers[socket];
    buffer.append(socket->readAll());

    // MBAP头部至少6字节
    while (buffer.size() >= 6) {
        // Protocol Identifier 必须为 0
        quint16 protocolId =
            (static_cast<quint8>(buffer[2]) << 8) |
            static_cast<quint8>(buffer[3]);

        if (protocolId != 0) {
            qWarning() << "非法Modbus TCP协议标识";
            buffer.clear();
            return;
        }

        // MBAP Length字段位于第4、5字节
        quint16 length =
            (static_cast<quint8>(buffer[4]) << 8) |
            static_cast<quint8>(buffer[5]);

        // Length包括Unit Identifier和PDU
        if (length < 2 || length > 254) {
            qWarning() << "非法Modbus TCP报文长度:" << length;
            buffer.clear();
            return;
        }

        // MBAP前6字节 + Length
        int frameLength = 6 + length;

        // 数据尚未接收完整，等待下一次readyRead
        if (buffer.size() < frameLength) {
            break;
        }

        // 提取一帧完整报文
        QByteArray frame = buffer.left(frameLength);
        buffer.remove(0, frameLength);

        qDebug() << "收到完整Modbus TCP请求:"
                 << frame.toHex(' ').toUpper();

        processRequest(socket, frame);
    }
}

void MockModbusTCPServer::processRequest(
    QTcpSocket *socket, const QByteArray &frame)
{
    if (!socket || frame.size() < 8) {
        return;
    }

    // 读取16位大端整数
    auto readUInt16 = [&frame](int offset) -> quint16 {
        return (static_cast<quint8>(frame[offset]) << 8) |
               static_cast<quint8>(frame[offset + 1]);
    };

    quint16 transactionId = readUInt16(0);
    quint8 unitId = static_cast<quint8>(frame[6]);
    quint8 functionCode = static_cast<quint8>(frame[7]);

    QByteArray response;

    // 暂时只支持读取保持寄存器（功能码03）
    if (functionCode != 0x03) {
        response.append(frame[0]);
        response.append(frame[1]);
        response.append(char(0));
        response.append(char(0));
        response.append(char(0));
        response.append(char(3));
        response.append(static_cast<char>(unitId));
        response.append(static_cast<char>(functionCode | 0x80));
        response.append(char(0x01)); // 非法功能码
        socket->write(response);
        return;
    }

    if (frame.size() != 12) {
        return;
    }

    quint16 startAddress = readUInt16(8);
    quint16 quantity = readUInt16(10);

    // 读取数量必须为1~125
    if (quantity < 1 || quantity > 125) {
        response.append(frame.left(4));
        response.append(char(0));
        response.append(char(3));
        response.append(static_cast<char>(unitId));
        response.append(char(0x83));
        response.append(char(0x03)); // 非法数据值
        socket->write(response);
        return;
    }

    // 模拟寄存器：温度、 电压、在线状态
    const QVector<quint16> registers = {256, 220, 1};

    // 当前模拟映射仅有地址0、1、2
    if (startAddress + quantity > registers.size()) {
        response.append(frame.left(4));
        response.append(char(0));
        response.append(char(3));
        response.append(static_cast<char>(unitId));
        response.append(char(0x83));
        response.append(char(0x02)); // 非法数据地址
        socket->write(response);
        return;
    }

    // 构造正常响应
    quint16 responseLength = 3 + quantity * 2;

    response.append(frame.left(4)); // Transaction ID + Protocol ID
    response.append(static_cast<char>(responseLength >> 8));
    response.append(static_cast<char>(responseLength & 0xFF));
    response.append(static_cast<char>(unitId));
    response.append(char(0x03));
    response.append(static_cast<char>(quantity * 2));

    for (int i = 0; i < quantity; ++i) {
        quint16 value = registers[startAddress + i];
        response.append(static_cast<char>(value >> 8));
        response.append(static_cast<char>(value & 0xFF));
    }

    socket->write(response);

    qDebug() << "Modbus TCP响应:"
             << response.toHex(' ').toUpper();
}
