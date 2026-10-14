#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "device.h"
#include "alarmmanager.h"
#include "devicerepository.h"
#include "../Communication/serialport.h"
#include "../Communication/modbustcp.h"
#include "../Core/deviceconfig.h"
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <QObject>
#include <QList>
#include <QVector>
#include <QThread>

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(DeviceRepository *deviceRepository, QObject *parent = nullptr);
    ~DeviceManager();
    bool addDevice(Device *device);
    void removeDevice(int deviceId);
    Device *getDevice(int deviceId);
    QList<Device *> devices() const;
    void restoreActiveAlarms(const QList<AlarmInfo> &alarms);
    void updateDeviceData(
        int deviceId,
        const DeviceData &data
        );

    void startSimulationDevices();
    void stopSimulationDevices();

    // 串口相关接口
    bool openSerialPort(
        const QString &portName,
        qint32 baudRate = QSerialPort::Baud9600,
        QSerialPort::DataBits dataBits = QSerialPort::Data8,
        QSerialPort::Parity parity = QSerialPort::NoParity,
        QSerialPort::StopBits stopBits = QSerialPort::OneStop
        );

    // Modbus RTU
    // Modbus RTU 读取保持寄存器
    bool requestModbusRead(
        quint8 slaveAddress,
        quint16 startAddress,
        quint16 quantity,
        int applicationDeviceId
        );

    // Modbus TCP
    bool connectModbusTCP(
        const QString &ip,
        quint16 port
        );
    void reconnectTCPDevices();

    bool requestModbusTCPRead(
        quint8 unitId,
        quint16 startAddress,
        quint16 quantity,
        int applicationDeviceId
        );

public slots:
    void updateAllDevices();
    void startPolling();
    void testTCPConnection(
        const QString &ip,
        quint16 port
        );
    void onModbusTimeout(quint16 transactionId);
signals:
    void deviceRemoved(int deviceId);

    void deviceDataUpdated(int deviceId,const DeviceData &data);
    void alarmTriggered(const AlarmInfo &alarm);

    // 串口相关信号
    void serialPortOpened();
    void serialPortClosed();
    void serialPortError(const QString &message);
    void deviceAdded(int deviceId);

    void connectTCPRequested(const QString &ip, quint16 port);
    void restartTCPRequested(const QString &ip, quint16 port);
    void sendTCPReadRequest(
        quint16 transactionId,
        quint8 unitId,
        quint16 startAddress,
        quint16 quantity
        );

    void tcpTestResult(
        bool success,
        QString message);
    void tcpConnectionChanged(bool connected, const QString &message);
private:
    // 设备注册表与借用的仓储接口；主线程服务由本对象管理，TCP 对象会移动到独立线程。
    QList<Device *> m_devices;
    DeviceRepository *m_deviceRepository = nullptr;
    AlarmManager *m_alarmManager = nullptr;
    SerialPort *m_serialPort = nullptr;

    // 共享串口总线配置，以及每台设备最近一次有效响应时间。
    ProtocolType m_protocolType = ProtocolType::Custom;
    DeviceConfig m_activeSerialConfig;
    bool m_hasActiveSerialConfig = false;
    QMap<int, QDateTime> m_lastReceivedTime;
    QTimer *m_timeoutTimer = nullptr;
    static constexpr int m_communicationTimeoutMs = 5000;
    bool openSerialForDevice(const DeviceConfig &config);
    void checkDeviceTimeout();

    // RTU 请求上下文：共享串口总线上同一时刻只维护一个待处理请求。
    QByteArray m_modbusBuffer;
    int m_expectedApplicationDeviceId = -1;
    quint8 m_expectedDeviceId = 0;
    quint16 m_expectedQuantity = 0;
    bool m_modbusRequestPending = false;
    QTimer *m_modbusTimeoutTimer = nullptr;
    static constexpr int m_modbusTimeoutMs = 3000;
    void handleModbusRawData(const QByteArray &data);
    void handleModbusRTUResponse(const QByteArray &data);
    void processModbusRegisters(int applicationDeviceId, const QVector<quint16> &registers);

    // TCP 套接字运行在 m_modbusThread 中；这些字段用于关联当前请求与响应。
    QThread *m_modbusThread = nullptr;
    ModbusTCP *m_modbusTCP = nullptr;
    QString m_tcpEndpointIp;
    quint16 m_tcpEndpointPort = 0;
    bool m_tcpConnected = false;
    bool m_tcpRequestPending = false;
    quint16 m_tcpExpectedTransactionId = 0;
    quint8 m_tcpExpectedUnitId = 0;
    quint16 m_tcpExpectedQuantity = 0;
    int m_tcpExpectedApplicationDeviceId = -1;
    QTimer *m_tcpTimeoutTimer = nullptr;
    void handleModbusTCPResponse(const QByteArray &data);

    // 刷新轮询列表时尽量保留当前位置，避免设备变更后跳过待轮询设备。
    QTimer *m_modbusPollTimer = nullptr;
    QList<int> m_pollDeviceIds;
    int m_currentPollIndex = 0;
    void refreshPollDeviceIds();
    void pollNextDevice();
    void markDeviceOffline(int deviceId, const QString &reason);

};

#endif // DEVICEMANAGER_H
