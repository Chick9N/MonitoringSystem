#ifndef ALARMMANAGER_H
#define ALARMMANAGER_H

#include <QObject>
#include "alarm.h"
#include "devicedata.h"
#include <QMap>
#include "deviceconfig.h"

class AlarmManager : public QObject
{
    Q_OBJECT
public:
    explicit AlarmManager(QObject *parent = nullptr);

    void checkAlarm(
        int deviceId,
        AlarmType type,
        bool abnormal,
        const QString &message);

    QList<AlarmInfo> checkDeviceData(
        int deviceId,
        const DeviceData &data,
        const DeviceConfig &config = DeviceConfig());
    void restoreActiveAlarms(const QList<AlarmInfo> &alarms);
    void forgetDevice(int deviceId);

signals:
    void alarmTriggered(const AlarmInfo &alarm);

private:
    // 警报去重
    QMap<int, QList<AlarmType>> m_activeAlarms; // 设备ID, 当前正在发生的报警
    bool isAlarmActive(
        int deviceId,
        AlarmType type
        ) const;

    void setAlarmActive(
        int deviceId,
        AlarmType type
        );

    void clearAlarm(
        int deviceId,
        AlarmType type
        );
};

#endif // ALARMMANAGER_H
