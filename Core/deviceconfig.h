#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H
#include <QString>

enum class ProtocolType
{
    Custom,
    ModbusRTU,
    ModbusTCP
};

enum class DataSource
{
    Simulation,
    Serial,
    TCP
};

struct DeviceConfig
{
    int deviceId = 0;
    QString deviceName;
    DataSource dataSource = DataSource::Simulation;
    ProtocolType protocolType = ProtocolType::Custom;

    // 串口配置
    QString serialPort;
    int baudRate = 9600;
    int dataBits = 8;
    int stopBits = 1;
    QString parity = "None";

    // Modbus RTU 配置
    int rtuSlaveId = 1;
    int rtuStartAddress = 0;
    int rtuQuantity = 3;

    // Modbus TCP 配置
    QString tcpIp;
    int tcpPort = 502;
    int tcpUnitId = 1;
    int tcpStartAddress = 0;
    int tcpQuantity = 3;
};

#endif // DEVICECONFIG_H
