#include "alarmmanager.h"

AlarmManager::AlarmManager(QObject *parent)
    : QObject(parent)
{
}

void AlarmManager::checkAlarm(
    int deviceId,
    AlarmType type,
    bool abnormal,
    const QString &message)
{
    if (abnormal)
    {
        if (isAlarmActive(deviceId, type))
            return;

        AlarmInfo alarm;
        alarm.deviceId = deviceId;
        alarm.type = type;
        alarm.message = message;
        alarm.timestamp = QDateTime::currentDateTime();

        setAlarmActive(deviceId, type);
        emit alarmTriggered(alarm);
    }
    else
    {
        if (!isAlarmActive(deviceId, type))
            return;

        AlarmInfo alarm;
        alarm.deviceId = deviceId;
        alarm.type = type;
        alarm.message = message + "，已恢复正常";
        alarm.timestamp = QDateTime::currentDateTime();
        alarm.recovered = true;

        clearAlarm(deviceId, type);

        emit alarmTriggered(alarm);
    }
}

QList<AlarmInfo> AlarmManager::checkDeviceData(
    int deviceId,
    const DeviceData &data,
    const DeviceConfig &config)
{
    QList<AlarmInfo> alarms;

    checkAlarm(
        deviceId,
        AlarmType::TemperatureHigh,
        data.temperature > config.temperatureHigh,
        "温度过高"
        );

    checkAlarm(
        deviceId,
        AlarmType::TemperatureLow,
        data.temperature < config.temperatureLow,
        "温度过低"
        );

    checkAlarm(
        deviceId,
        AlarmType::VoltageHigh,
        data.voltage > config.voltageHigh,
        "电压过高"
        );

    checkAlarm(
        deviceId,
        AlarmType::VoltageLow,
        data.voltage < config.voltageLow,
        "电压过低"
        );

    checkAlarm(
        deviceId,
        AlarmType::DeviceOffline,
        !data.isOnline,
        "设备离线"
        );

    return alarms;
}

void AlarmManager::restoreActiveAlarms(const QList<AlarmInfo> &alarms)
{
    m_activeAlarms.clear();
    for (const AlarmInfo &alarm : alarms)
        setAlarmActive(alarm.deviceId, alarm.type);
}

void AlarmManager::forgetDevice(int deviceId)
{
    m_activeAlarms.remove(deviceId);
}

bool AlarmManager::isAlarmActive(
    int deviceId,
    AlarmType type) const
{
    return m_activeAlarms
        .value(deviceId)
        .contains(type);
}

void AlarmManager::setAlarmActive(
    int deviceId,
    AlarmType type)
{
    if (!m_activeAlarms[deviceId].contains(type))
    {
        m_activeAlarms[deviceId].append(type);
    }
}

void AlarmManager::clearAlarm(
    int deviceId,
    AlarmType type)
{
    if (!m_activeAlarms.contains(deviceId))
        return;

    m_activeAlarms[deviceId].removeAll(type);

    if (m_activeAlarms[deviceId].isEmpty())
    {
        m_activeAlarms.remove(deviceId);
    }
}
