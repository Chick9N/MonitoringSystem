#ifndef ALARM_H
#define ALARM_H

#include <QString>
#include <QDateTime>

enum class AlarmType
{
    TemperatureHigh,
    TemperatureLow,
    VoltageHigh,
    VoltageLow,
    DeviceOffline
};

struct AlarmInfo
{
    int deviceId = -1;
    AlarmType type;
    QString message;
    QDateTime timestamp;
    bool recovered = false;
};

inline QString alarmTypeToString(AlarmType type)
{
    switch (type)
    {
    case AlarmType::TemperatureHigh:
    case AlarmType::TemperatureLow:
        return "温度";

    case AlarmType::VoltageHigh:
    case AlarmType::VoltageLow:
        return "电压";

    case AlarmType::DeviceOffline:
        return "设备状态";
    }

    return "未知";
}

#endif // ALARM_H
