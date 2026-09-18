#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QDateTime>
#include "../Core/devicedata.h"


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
    void closeDatabase();

    bool createTables();

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

private:

    QSqlDatabase m_database;
};

#endif // DATABASEMANAGER_H
