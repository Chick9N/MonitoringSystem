#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QDateTime>
#include "../Core/devicedata.h"
#include "../Core/alarm.h"
#include "../Core/deviceconfig.h"

// 封装一条数据库历史记录
struct DeviceHistory
{
    DeviceHistory() {}
    int id;
    int deviceId;
    DeviceData data;
    QDateTime timestamp;
};

class DatabaseManager : public QObject
{
    Q_OBJECT;

public:
    explicit DatabaseManager(QObject *parent = nullptr);

    bool openDatabase();
    QString databasePath() const;
    void closeDatabase();

    bool createTables();

    // 设备配置管理
    // 返回数据库自动分配的设备 ID，失败返回 -1
    int insertDevice(const DeviceConfig &config);

    // 查询所有已保存的设备配置
    QList<DeviceConfig> queryDevices();

    // 删除设备配置
    bool deleteDevice(int deviceId);

    // 判断设备是否存在
    bool deviceExists(int deviceId);

    bool insertDeviceData(int deviceId, const DeviceData &data);

    // 查询某设备全部历史数据
    QList<DeviceHistory> queryDeviceHistory(int deviceId);

    // 查询某设备指定时间范围的数据
    QList<DeviceHistory> queryDeviceHistory(
        int deviceId,
        const QDateTime &startTime,
        const QDateTime &endTime);

    // 查询某设备最新一条数据
    DeviceHistory queryLatestDeviceData(int deviceId);

    // 删除某设备全部历史数据
    bool deleteDeviceHistory(int deviceId);

    void insertAlarm(const AlarmInfo &alarm);
    QList<AlarmInfo> queryAlarmHistory();
    QList<AlarmInfo> queryAlarmHistory(int limit);
    QList<AlarmInfo> queryActiveAlarms();
    void deleteAlarmHistory();
private:

    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H
