#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>


DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent){}

bool DatabaseManager::openDatabase(){
    m_database =
        QSqlDatabase::addDatabase("QSQLITE");


    m_database.setDatabaseName(
        "device.db"
        );


    if(!m_database.open())
    {
        qDebug()
        << "数据库打开失败:"
        << m_database.lastError();

        return false;
    }


    qDebug()
        << "数据库连接成功";

    return true;
}

void DatabaseManager::closeDatabase()
{
    if (m_database.isOpen())
    {
        m_database.close();
    }
}

bool DatabaseManager::createTables()
{
    QSqlQuery query;

    QString createTableSql = R"(
        CREATE TABLE IF NOT EXISTS device_history
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id INTEGER NOT NULL,
            temperature REAL NOT NULL,
            voltage REAL NOT NULL,
            online INTEGER NOT NULL,
            timestamp DATETIME NOT NULL
        )
    )";

    if (!query.exec(createTableSql))
    {
        qDebug() << "创建数据表失败:"
                 << query.lastError();

        return false;
    }

    QString createIndexSql = R"(
        CREATE INDEX IF NOT EXISTS
        idx_device_history_device_time
        ON device_history(device_id, timestamp)
    )";

    if (!query.exec(createIndexSql))
    {
        qDebug() << "创建索引失败:"
                 << query.lastError();

        return false;
    }

    return true;
}

bool DatabaseManager::insertDeviceData(int deviceId, const DeviceData &data){
    QSqlQuery query;

    query.prepare(R"(
        INSERT INTO device_history
        (
            device_id,
            temperature,
            voltage,
            online,
            timestamp
        )
        VALUES
        (
            :device_id,
            :temperature,
            :voltage,
            :online,
            :timestamp
        )
    )");

    query.bindValue(":device_id", deviceId);
    query.bindValue(":temperature", data.temperature);
    query.bindValue(":voltage", data.voltage);
    query.bindValue(":online", data.isOnline);
    query.bindValue(
        ":timestamp",
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
        );

    if (!query.exec())
    {
        qDebug() << "设备数据插入失败:"
                 << query.lastError();

        return false;
    }

    return true;
}

QList<DeviceHistory> DatabaseManager::queryDeviceHistory(int deviceId)
{
    QList<DeviceHistory> historyList;

    QSqlQuery query;

    query.prepare(R"(
        SELECT
            id,
            device_id,
            temperature,
            voltage,
            online,
            timestamp
        FROM device_history
        WHERE device_id = :device_id
        ORDER BY timestamp ASC
    )");

    query.bindValue(":device_id", deviceId);

    if (!query.exec())
    {
        qDebug() << "查询设备历史数据失败:"
                 << query.lastError();

        return historyList;
    }

    while (query.next())
    {
        DeviceHistory history;

        history.id = query.value("id").toInt();
        history.deviceId = query.value("device_id").toInt();

        history.data.temperature =
            query.value("temperature").toDouble();

        history.data.voltage =
            query.value("voltage").toDouble();

        history.data.isOnline =
            query.value("online").toBool();

        history.timestamp =
            QDateTime::fromString(
                query.value("timestamp").toString(),
                "yyyy-MM-dd HH:mm:ss"
                );

        historyList.append(history);
    }

    return historyList;
}

QList<DeviceHistory> DatabaseManager::queryDeviceHistory(
    int deviceId,
    const QDateTime &startTime,
    const QDateTime &endTime)
{
    QList<DeviceHistory> historyList;

    QSqlQuery query;

    query.prepare(R"(
        SELECT
            id,
            device_id,
            temperature,
            voltage,
            online,
            timestamp
        FROM device_history
        WHERE device_id = :device_id
          AND timestamp >= :start_time
          AND timestamp <= :end_time
        ORDER BY timestamp ASC
    )");

    query.bindValue(":device_id", deviceId);
    query.bindValue(":start_time", startTime.toString("yyyy-MM-dd HH:mm:ss"));
    query.bindValue(":end_time", endTime.toString("yyyy-MM-dd HH:mm:ss"));

    if (!query.exec())
    {
        qDebug() << "查询时间范围数据失败:"
                 << query.lastError();

        return historyList;
    }

    while (query.next())
    {
        DeviceHistory history;

        history.id = query.value("id").toInt();
        history.deviceId = query.value("device_id").toInt();

        history.data.temperature =
            query.value("temperature").toDouble();

        history.data.voltage =
            query.value("voltage").toDouble();

        history.data.isOnline =
            query.value("online").toBool();

        history.timestamp =
            QDateTime::fromString(
                query.value("timestamp").toString(),
                "yyyy-MM-dd HH:mm:ss"
                );

        historyList.append(history);
    }

    return historyList;
}

DeviceHistory DatabaseManager::queryLatestDeviceData(int deviceId)
{
    DeviceHistory history;

    QSqlQuery query;

    query.prepare(R"(
        SELECT
            id,
            device_id,
            temperature,
            voltage,
            online,
            timestamp
        FROM device_history
        WHERE device_id = :device_id
        ORDER BY timestamp DESC
        LIMIT 1
    )");

    query.bindValue(":device_id", deviceId);

    if (!query.exec())
    {
        qDebug() << "查询最新设备数据失败:"
                 << query.lastError();

        return history;
    }

    if (query.next())
    {
        history.id = query.value("id").toInt();
        history.deviceId = query.value("device_id").toInt();

        history.data.temperature =
            query.value("temperature").toDouble();

        history.data.voltage =
            query.value("voltage").toDouble();

        history.data.isOnline =
            query.value("online").toBool();

        history.timestamp =
            QDateTime::fromString(
                query.value("timestamp").toString(),
                "yyyy-MM-dd HH:mm:ss"
                );
    }

    return history;
}

bool DatabaseManager::deleteDeviceHistory(int deviceId)
{
    QSqlQuery query;

    query.prepare(R"(
        DELETE FROM device_history
        WHERE device_id = :device_id
    )");

    query.bindValue(":device_id", deviceId);

    if (!query.exec())
    {
        qDebug() << "删除设备历史数据失败:"
                 << query.lastError();

        return false;
    }

    return true;
}

