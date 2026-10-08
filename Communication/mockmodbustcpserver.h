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
#include <QString>

/*
 *  实际工业设备通信中，通常由你的上位机作为 Modbus TCP 客户端，主动连接设备提供的服务器。
 *  因此这里建立一个假服务器，上位机系统作为客户端，接收传来的数据。
 */


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
    bool isRunning() const;
    QString errorString() const;
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
    QString m_lastError;

};

#endif // MOCKMODBUSTCPSERVER_H
