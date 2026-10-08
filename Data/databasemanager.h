#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QDateTime>
#include "../Core/devicedata.h"
#include "../Core/alarm.h"
#include "../Core/deviceconfig.h"
#include "../Core/devicerepository.h"

// 封装一条数据库历史记录
struct DeviceHistory
{
    DeviceHistory() {}
    int id;
    int deviceId;
    DeviceData data;
    QDateTime timestamp;
};

struct DatabaseSnapshot
{
    QList<DeviceConfig> devices;
    QList<AlarmInfo> alarmHistory;
    QList<AlarmInfo> activeAlarms;
};

class DatabaseManager : public QObject, public DeviceRepository
{
    Q_OBJECT;

public:
    explicit DatabaseManager(QObject *parent = nullptr);

    bool openDatabase();
    QString databasePath() const;
    void closeDatabase();

    bool createTables();
    static DatabaseSnapshot loadStartupSnapshot(const QString &databasePath);

    // 设备配置管理
    // 返回当前最小可用设备 ID，失败返回 -1
    int insertDevice(const DeviceConfig &config);

    // 删除设备配置
    bool deleteDevice(int deviceId) override;

    // 查询某设备全部历史数据
    QList<DeviceHistory> queryDeviceHistory(int deviceId);

    // 查询某设备指定时间范围的数据
    QList<DeviceHistory> queryDeviceHistory(
        int deviceId,
        const QDateTime &startTime,
        const QDateTime &endTime);

    void insertAlarm(const AlarmInfo &alarm);
    void deleteAlarmHistory();
private:
    friend class DatabaseWriteQueue;
    static bool insertDeviceData(
        QSqlDatabase &database,
        int deviceId,
        const DeviceData &data);

    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H
