#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QStringList>

class SerialPort : public QObject
{
    Q_OBJECT

public:
    explicit SerialPort(QObject *parent = nullptr);

    bool open(
        const QString &portName,
        qint32 baudRate = QSerialPort::Baud9600,
        QSerialPort::DataBits dataBits = QSerialPort::Data8,
        QSerialPort::Parity parity = QSerialPort::NoParity,
        QSerialPort::StopBits stopBits = QSerialPort::OneStop
        );

    void close();

    bool isOpen() const;

    bool sendData(const QByteArray &data);

    static QStringList availablePorts();

    QString errorString() const;

    void setModbusMode(bool enable);

    // 测试入口
    void simulateReceive(const QByteArray &data);
    void simulateRawReceive(const QByteArray &data);

signals:
    // 用于后续 Modbus RTU 等协议
    void rawDataReceived(const QByteArray &data);
    // 用于现有自定义协议解析
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &message);
    void opened();
    void closed();

private slots:
    void readData();

private:
    QSerialPort *m_serialPort;
    QByteArray m_buffer; // 缓冲区

    bool m_modbusMode=false;

    // 帧解析
    void processBuffer();
};

#endif // SERIALPORT_H
