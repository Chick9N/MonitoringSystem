#ifndef MOCKMODBUSTCPSERVER_H
#define MOCKMODBUSTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QHostAddress>
#include <QHash>

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

private:
    // 监听本地 TCP 端口
    QTcpServer *m_server;
    QHash<QTcpSocket*, QByteArray> m_clientBuffers;
};

#endif // MOCKMODBUSTCPSERVER_H
