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
    Device* getDevice(int deviceId);
    QList<Device*> devices() const;
    void updateDeviceData(
        int deviceId,
        const DeviceData &data
        );

    void startAll();
    void stopAll();

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
    void sendTCPReadRequest(
        quint16 transactionId,
        quint8 unitId,
        quint16 startAddress,
        quint16 quantity
        );

    void tcpTestResult(
        bool success,
        QString message);
private:
    QList<Device*> m_devices; // 设备容器
    AlarmManager *m_alarmManager;
    SerialPort *m_serialPort;

    QMap<int, QDateTime> m_lastReceivedTime;
    QTimer *m_timeoutTimer = nullptr;

    static constexpr int CommunicationTimeoutMs = 5000;

    ProtocolType m_protocolType = ProtocolType::Custom;
    DeviceConfig m_activeSerialConfig;
    bool m_hasActiveSerialConfig = false;

    void handleModbusRTUResponse(
        const QByteArray &data
        );


    void handleModbusTCPResponse(
        const QByteArray &data
        );
    // 数据库
    DeviceRepository *m_deviceRepository = nullptr;

    // 自定义串口协议
    bool openSerialForDevice(const DeviceConfig &config);

    // Modbus
    // Modbus RTU 接收缓冲区
    QByteArray m_modbusBuffer;

    // 当前 RTU 请求对应的应用层设备 ID
    int m_expectedApplicationDeviceId = -1;

    // 当前待处理请求的信息
    quint8 m_expectedDeviceId = 0;
    quint16 m_expectedQuantity = 0;
    bool m_modbusRequestPending = false;

    // Modbus 请求超时定时器
    QTimer *m_modbusTimeoutTimer = nullptr;

    // Modbus 请求超时时间（毫秒）
    static constexpr int ModbusTimeoutMs = 3000;

    void processModbusRegisters(
        int applicationDeviceId,
        const QVector<quint16> &registers);

    // TCP
    bool m_tcpConnected = false;
    ModbusTCP *m_modbusTCP;

    // Modbus TCP 请求状态
    bool m_tcpRequestPending = false;
    quint16 m_tcpExpectedTransactionId = 0;
    quint8 m_tcpExpectedUnitId = 0;
    quint16 m_tcpExpectedQuantity = 0;

    // 当前 TCP 请求对应的应用层设备 ID
    int m_tcpExpectedApplicationDeviceId = -1;

    // 超时
    QTimer *m_tcpTimeoutTimer = nullptr;
    // 轮询
    QTimer *m_modbusPollTimer = nullptr;
    QList<int> m_pollDeviceIds;
    int m_currentPollIndex = 0;
    void refreshPollDeviceIds();
    void pollNextDevice();

    void handleModbusRawData(const QByteArray &data);

    // 多线程
    QThread *m_modbusThread = nullptr;

private slots:
    void checkDeviceTimeout();
};

#endif // DEVICEMANAGER_H
