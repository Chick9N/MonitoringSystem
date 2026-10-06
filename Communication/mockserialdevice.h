#ifndef MOCKSERIALDEVICE_H
#define MOCKSERIALDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QByteArray>
/*
 * 自定义串口的mock
 * 使用外部软件打开Windows虚拟com bridge, COM1<->COM2
 * 下位机使用COM1, 上位机使用COM2
 */
class MockSerialDevice : public QObject
{
    Q_OBJECT

public:
    explicit MockSerialDevice(QObject *parent = nullptr);
    ~MockSerialDevice();

    // 打开模拟设备串口
    bool start(
        const QString &portName,
        qint32 baudRate = QSerialPort::Baud9600
        );

    // 关闭模拟设备
    void stop();

    bool isRunning() const;

signals:

    void started();
    void stopped();

private slots:

    // 串口收到数据
    void readData();

    // 定时更新模拟设备数据
    void updateMockData();

private:

    // 处理上位机发送的请求
    void processRequest(const QByteArray &request);

    // 向上位机发送响应
    void sendResponse(const QByteArray &response);

private:

    QSerialPort *m_serialPort;

    QTimer *m_dataTimer;

    QByteArray m_buffer;

    // 模拟设备数据
    quint16 m_temperature;
    quint16 m_voltage;
    quint16 m_online;
};

#endif // MOCKSERIALDEVICE_H
