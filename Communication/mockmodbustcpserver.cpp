#include "mockmodbustcpserver.h"
#include "../Utils/simulatedata.h"
#include <QDebug>
#include <QVector>

MockModbusTCPServer::MockModbusTCPServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_updateTimer(new QTimer(this))
{
    // 模拟设备1
    MockDeviceData dev1;

    dev1.unitId = 1;

    dev1.registers =
        {
            {0,256},   // 温度25.6℃
            {1,220},   // 电压
            {2,1}      // 在线
        };


    // 模拟设备2
    MockDeviceData dev2;

    dev2.unitId = 2;

    dev2.registers =
        {
            {0,315},
            {1,221},
            {2,1}
        };


    // 模拟设备3
    MockDeviceData dev3;

    dev3.unitId = 3;

    dev3.registers =
        {
            {0,280},
            {1,218},
            {2,1}
        };


    m_devices.insert(1,dev1);
    m_devices.insert(2,dev2);
    m_devices.insert(3,dev3);



    connect(
        m_server,
        &QTcpServer::newConnection,
        this,
        &MockModbusTCPServer::onNewConnection
        );

    connect(
        m_updateTimer,
        &QTimer::timeout,
        this,
        &MockModbusTCPServer::updateMockDevices
        );


    // 每1000ms更新一次模拟数据
    m_updateTimer->start(1000);
}

bool MockModbusTCPServer::start(quint16 port)
{
    if (m_server->isListening()) {
        return true;
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        m_lastError = m_server->errorString();
        qWarning() << "Modbus TCP服务器启动失败:"
                   << m_lastError;
        return false;
    }

    m_lastError.clear();

    qDebug() << "Modbus TCP服务器已启动，端口:"
             << m_server->serverPort();

    return true;
}

bool MockModbusTCPServer::isRunning() const
{
    return m_server->isListening();
}

QString MockModbusTCPServer::errorString() const
{
    return m_lastError;
}

void MockModbusTCPServer::stop()
{

    // 断开所有已经连接的客户端
    for(auto socket : m_clientBuffers.keys())
    {
        if(socket)
        {
            socket->abort();

            socket->deleteLater();
        }
    }


    m_clientBuffers.clear();


    // 停止监听
    if(m_server->isListening())
    {
        m_server->close();
    }


    qDebug()
        << "Modbus TCP服务器已停止";

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
    QTcpSocket *socket,
    const QByteArray &frame)
{
    if (!socket || frame.size() < 8)
    {
        return;
    }


    /*
        Modbus TCP报文格式:

        MBAP Header:
        --------------------------------
        Transaction ID  2字节
        Protocol ID     2字节
        Length          2字节

        Unit ID          1字节

        PDU:
        Function Code    1字节
        Data             N字节

    */


    // 读取16位大端数据
    auto readUInt16 =
        [&frame](int offset)->quint16
    {
        return
            (static_cast<quint8>(frame[offset]) << 8)
            |
            static_cast<quint8>(frame[offset+1]);
    };


    // ================================
    // 解析MBAP头
    // ================================

    quint16 transactionId = readUInt16(0);

    quint16 protocolId = readUInt16(2);

    quint16 length = readUInt16(4);


    quint8 unitId =
        static_cast<quint8>(frame[6]);


    quint8 functionCode =
        static_cast<quint8>(frame[7]);



    // Modbus TCP协议标识必须为0
    if(protocolId != 0)
    {
        qWarning()
        <<"非法Protocol ID";

        return;
    }


    // 检查报文长度
    if(length != frame.size()-6)
    {
        qWarning()
        <<"Modbus TCP长度错误";

        return;
    }



    /*
        发送异常响应

        异常响应格式:

        Transaction ID
        Protocol ID
        Length
        Unit ID
        Function Code + 0x80
        Exception Code

    */

    auto sendException =
        [&](quint8 code)
    {
        QByteArray response;


        // 保留事务ID和协议ID
        response.append(frame.left(4));


        // Length:
        // UnitID + FunctionCode + ExceptionCode
        response.append(char(0));
        response.append(char(3));


        response.append(
            static_cast<char>(unitId)
            );


        response.append(
            static_cast<char>(functionCode | 0x80)
            );


        response.append(
            static_cast<char>(code)
            );


        socket->write(response);


        qDebug()
            <<"Modbus TCP异常响应:"
            <<response.toHex(' ').toUpper();
    };




    // ================================
    // 检查功能码
    // 当前只支持03读取保持寄存器
    // ================================

    if(functionCode != 0x03)
    {
        // Illegal Function
        sendException(0x01);

        return;
    }



    /*
        FC03请求格式:

        Unit ID
        Function Code
        Start Address 2字节
        Quantity      2字节

        所以完整长度:
        MBAP 6字节
        + UnitID 1
        + PDU 5

        总共12字节
    */

    if(frame.size()!=12)
    {
        // Illegal Data Value
        sendException(0x03);

        return;
    }



    quint16 startAddress =
        readUInt16(8);


    quint16 quantity =
        readUInt16(10);



    // Modbus规定一次最多125个寄存器
    if(quantity < 1 || quantity > 125)
    {
        sendException(0x03);

        return;
    }




    // ================================
    // 根据UnitID寻找模拟设备
    // ================================

    if(!m_devices.contains(unitId))
    {
        qWarning()
        <<"不存在模拟设备 UnitID:"
        <<unitId;


        // Illegal Data Address
        sendException(0x02);

        return;
    }



    const MockDeviceData &device =
        m_devices[unitId];



    /*
        根据请求地址读取寄存器

        例如:

        startAddress=0
        quantity=3


        读取:

        0 温度
        1 电压
        2 在线状态

    */


    QVector<quint16> values;


    for(int i=0;i<quantity;i++)
    {

        quint16 address =
            startAddress+i;



        // 模拟设备不存在该寄存器
        if(!device.registers.contains(address))
        {
            qWarning()
            <<"不存在寄存器地址:"
            <<address;


            // Illegal Data Address
            sendException(0x02);

            return;
        }



        values.append(
            device.registers[address]
            );
    }




    // ================================
    // 构造正常响应
    // ================================


    QByteArray response;



    /*
        响应:

        Transaction ID
        Protocol ID
        Length
        Unit ID
        Function Code
        Byte Count
        Register Data

    */


    response.append(
        frame.left(4)
        );



    // Length =
    // UnitID(1)
    // FunctionCode(1)
    // ByteCount(1)
    // Data(quantity*2)

    quint16 responseLength =
        3 + quantity * 2;



    response.append(
        static_cast<char>(responseLength >> 8)
        );


    response.append(
        static_cast<char>(responseLength & 0xff)
        );



    response.append(
        static_cast<char>(unitId)
        );


    response.append(
        char(0x03)
        );



    // 数据字节数量
    response.append(
        static_cast<char>(quantity*2)
        );



    // 写入寄存器数据
    for(quint16 value: values)
    {
        response.append(
            static_cast<char>(value >> 8)
            );

        response.append(
            static_cast<char>(value & 0xff)
            );
    }



    socket->write(response);



    qDebug()
        <<"Modbus TCP响应:"
        <<response.toHex(' ').toUpper();


    qDebug()
        <<"设备UnitID:"
        <<unitId
        <<"寄存器:"
        <<values;
}

void MockModbusTCPServer::updateMockDevices()
{

    for(auto &device : m_devices)
    {

        // 温度寄存器 地址0

        device.registers[0] =
            SimulationData::temperature(
                device.registers[0]
                );


        // 电压寄存器 地址1

        device.registers[1] =
            SimulationData::voltage(
                device.registers[1]
                );


        // 在线状态 地址2

        device.registers[2] =
            SimulationData::online();

    }

}
