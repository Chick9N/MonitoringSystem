#ifndef MOCKMODBUSRTUDEVICE_H
#define MOCKMODBUSRTUDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>

class MockModbusRTUDevice : public QObject
{
    Q_OBJECT

public:
    explicit MockModbusRTUDevice(QObject *parent = nullptr);

    bool start(
        const QString &portName,
        qint32 baudRate = 9600
        );

    void stop();

    bool isRunning() const;

signals:
    void started();
    void stopped();
    void errorOccurred(const QString &message);

private slots:
    void readData();
    void updateMockData();

private:
    void processRequest(const QByteArray &request);

    QByteArray buildResponse(
        quint8 slaveAddress,
        quint16 startAddress,
        quint16 quantity
        );

    quint16 calculateCRC(const QByteArray &data);

    void sendResponse(const QByteArray &response);

private:
    QSerialPort *m_serialPort;
    QTimer *m_dataTimer;

    QByteArray m_buffer;

    quint16 m_temperature = 253;
    quint16 m_voltage = 220;
    quint16 m_online = 1;
};

#endif // MOCKMODBUSRTUDEVICE_H
