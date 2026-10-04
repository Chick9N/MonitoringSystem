#ifndef MOCKMODBUSTCPSERVER_H
#define MOCKMODBUSTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QHash>
#include <QMap>
#include <QTimer>


struct MockDeviceData
{
    quint8 unitId;

    // 地址 -> 数据
    QMap<quint16, quint16> registers;
};

class MockModbusTCPServer : public QObject
{
    Q_OBJECT
public:
    explicit MockModbusTCPServer(QObject *parent = nullptr);
    // 启动和停止模拟服务器
    bool start(quint16 port = 1502);
    void stop();
    void processRequest(QTcpSocket *socket, const QByteArray &frame);

signals:

private slots:
    // 接受客户端连接
    void onNewConnection();
    // 接收客户端发送的 Modbus TCP 请求
    void readRequest();
    // 定时更新设备数据
    void updateMockDevices();
private:
    // 监听本地 TCP 端口
    QTcpServer *m_server;
    QHash<QTcpSocket*, QByteArray> m_clientBuffers;

    // 模拟数据
    QMap<quint8, MockDeviceData> m_devices;

    QTimer *m_updateTimer;

};

#endif // MOCKMODBUSTCPSERVER_H
