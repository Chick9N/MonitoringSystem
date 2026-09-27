#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QByteArray>

#include "../Core/devicedata.h"

class ProtocolParser
{
public:
    static bool parse(
        const QByteArray &data,
        int &deviceId,
        DeviceData &deviceData
        );
};

#endif // PROTOCOLPARSER_H

// 模拟一帧(7字节)
// AA 设备ID 温度 电压高字节 电压低字节 在线状态 55
// AA 01 20 00 DC 01 55
// AA       帧头
// 01       设备ID = 1
// 20       温度 = 32℃
// 00 DC    电压 = 220V
// 01       在线
// 55       帧尾
