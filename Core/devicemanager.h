#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QList>
#include "device.h"
#include "alarmmanager.h"
#include "../Communication/serialport.h"

#include <QDateTime>
#include <QMap>
#include <QTimer>

enum class DataSource
{
    Simulation,
    Serial
};

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    void addDevice(Device *device);
    Device* getDevice(int deviceId);
    QList<Device*> devices() const;
    void updateDeviceData(
        int deviceId,
        const DeviceData &data
        );

    void startAll();
    void stopAll();

    // 串口相关接口
    QStringList availableSerialPorts() const;

    bool openSerialPort(
        const QString &portName,
        qint32 baudRate = QSerialPort::Baud9600,
        QSerialPort::DataBits dataBits = QSerialPort::Data8,
        QSerialPort::Parity parity = QSerialPort::NoParity,
        QSerialPort::StopBits stopBits = QSerialPort::OneStop
        );

    void closeSerialPort();

    bool isSerialPortOpen() const;

    // 信源
    void setDataSource(DataSource source);
    DataSource dataSource() const;

    // 测试入口
     void simulateSerialData(const QByteArray &data);
public slots:
    void updateAllDevices();

signals:
    void deviceDataUpdated(int deviceId,const DeviceData &data);
    void alarmTriggered(const AlarmInfo &alarm);

    // 串口相关信号
    void serialPortOpened();
    void serialPortClosed();
    void serialPortError(const QString &message);
    void deviceAdded(int deviceId);
private:
    QList<Device*> m_devices; // 设备容器
    AlarmManager *m_alarmManager;
    SerialPort *m_serialPort;
    DataSource m_dataSource = DataSource::Simulation;

    QMap<int, QDateTime> m_lastReceivedTime;
    QTimer *m_timeoutTimer = nullptr;

    static constexpr int CommunicationTimeoutMs = 5000;

private slots:
    void checkDeviceTimeout();
};

#endif // DEVICEMANAGER_H
