#include "protocolparser.h"

bool ProtocolParser::parse(
    const QByteArray &data,
    int &deviceId,
    DeviceData &deviceData)
{
    // 一帧固定 7 字节
    if (data.size() != 7)
        return false;

    // 检查帧头
    if (static_cast<unsigned char>(data[0]) != 0xAA)
        return false;

    // 检查帧尾
    if (static_cast<unsigned char>(data[6]) != 0x55)
        return false;

    // 设备 ID
    deviceId =
        static_cast<unsigned char>(data[1]);

    // 温度
    deviceData.temperature =
        static_cast<unsigned char>(data[2]);

    // 电压
    int voltage =
        (static_cast<unsigned char>(data[3]) << 8)
        |
        static_cast<unsigned char>(data[4]);

    deviceData.voltage = voltage;

    // 在线状态
    deviceData.isOnline =
        static_cast<unsigned char>(data[5]) != 0;

    return true;
}

