#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <algorithm>
DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent){}

QString DatabaseManager::databasePath() const
{
    return m_database.databaseName();
}

DatabaseSnapshot DatabaseManager::loadStartupSnapshot(const QString &databasePath)
{
    DatabaseSnapshot snapshot;
    const QString connectionName = QStringLiteral("startup_read_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(databasePath);
        if (!db.open()) {
            qWarning() << "后台打开数据库失败:" << db.lastError();
        } else {
            {
                QSqlQuery query(db);
                const QString sql = R"(
                    SELECT device_id, device_name, data_source, protocol_type,
                           serial_port, baud_rate, data_bits, stop_bits, parity,
                           rtu_slave_id, rtu_start_address, rtu_quantity,
                           tcp_ip, tcp_port, tcp_unit_id, tcp_start_address, tcp_quantity
                    FROM device
                    ORDER BY device_id
                )";
                if (!query.exec(sql)) {
                    qWarning() << "后台查询设备失败:" << query.lastError();
                } else {
                    while (query.next()) {
                        DeviceConfig config;
                        config.deviceId = query.value(0).toInt();
                        config.deviceName = query.value(1).toString();
                        config.dataSource = static_cast<DataSource>(query.value(2).toInt());
                        config.protocolType = static_cast<ProtocolType>(query.value(3).toInt());
                        config.serialPort = query.value(4).toString();
                        config.baudRate = query.value(5).toInt();
                        config.dataBits = query.value(6).toInt();
                        config.stopBits = query.value(7).toInt();
                        config.parity = query.value(8).toString();
                        config.rtuSlaveId = query.value(9).toInt();
                        config.rtuStartAddress = query.value(10).toInt();
                        config.rtuQuantity = query.value(11).toInt();
                        config.tcpIp = query.value(12).toString();
                        config.tcpPort = query.value(13).toInt();
                        config.tcpUnitId = query.value(14).toInt();
                        config.tcpStartAddress = query.value(15).toInt();
                        config.tcpQuantity = query.value(16).toInt();
                        snapshot.devices.append(config);
                    }
                }
            }

            auto readAlarms = [&db](const QString &sql, QList<AlarmInfo> &alarms) {
                QSqlQuery query(db);
                query.prepare(sql);
                if (sql.contains(":limit"))
                    query.bindValue(":limit", 500);
                if (!query.exec()) {
                    qWarning() << "后台查询报警失败:" << query.lastError();
                    return;
                }
                while (query.next()) {
                    AlarmInfo alarm;
                    alarm.deviceId = query.value(0).toInt();
                    alarm.type = static_cast<AlarmType>(query.value(1).toInt());
                    alarm.message = query.value(2).toString();
                    alarm.recovered = query.value(3).toInt() != 0;
                    alarm.timestamp = QDateTime::fromString(
                        query.value(4).toString(), "yyyy-MM-dd HH:mm:ss");
                    alarms.append(alarm);
                }
            };

            readAlarms(R"(
                SELECT device_id, alarm_type, message, recovered, timestamp
                FROM alarm_history
                ORDER BY id DESC
                LIMIT :limit
            )", snapshot.alarmHistory);
            std::reverse(snapshot.alarmHistory.begin(), snapshot.alarmHistory.end());

            readAlarms(R"(
                SELECT a.device_id, a.alarm_type, a.message, a.recovered, a.timestamp
                FROM alarm_history a
                WHERE a.recovered = 0
                  AND a.id = (
                      SELECT MAX(b.id)
                      FROM alarm_history b
                      WHERE b.device_id = a.device_id
                        AND b.alarm_type = a.alarm_type
                  )
                ORDER BY a.id ASC
            )", snapshot.activeAlarms);
        }
        db.close();
    }

    QSqlDatabase::removeDatabase(connectionName);
    return snapshot;
}

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

    // 1. 创建设备配置表
    QString createDeviceTableSql = R"(
        CREATE TABLE IF NOT EXISTS device
        (
            device_id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_name TEXT NOT NULL,
            data_source INTEGER NOT NULL,
            protocol_type INTEGER NOT NULL,

            serial_port TEXT,
            baud_rate INTEGER,
            data_bits INTEGER,
            stop_bits INTEGER,
            parity TEXT,

            rtu_slave_id INTEGER,
            rtu_start_address INTEGER,
            rtu_quantity INTEGER,

            tcp_ip TEXT,
            tcp_port INTEGER,
            tcp_unit_id INTEGER,
            tcp_start_address INTEGER,
            tcp_quantity INTEGER
        )
    )";

    if (!query.exec(createDeviceTableSql))
    {
        qDebug() << "创建设备配置表失败:"
                 << query.lastError();

        return false;
    }

    // 2. 创建设备历史数据表
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

    // 3. 创建设备历史数据索引
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

    // 4. 创建报警历史表
    QString createAlarmTableSql = R"(
        CREATE TABLE IF NOT EXISTS alarm_history
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id INTEGER NOT NULL,
            alarm_type INTEGER NOT NULL,
            message TEXT NOT NULL,
            recovered INTEGER NOT NULL,
            timestamp DATETIME NOT NULL
        )
    )";

    if (!query.exec(createAlarmTableSql))
    {
        qDebug() << "创建报警历史表失败:"
                 << query.lastError();

        return false;
    }


    // 5. 创建报警历史数据索引
    QString createAlarmIndexSql = R"(
    CREATE INDEX IF NOT EXISTS
    idx_alarm_history_device_time
    ON alarm_history(device_id, timestamp)
)";


    if (!query.exec(createAlarmIndexSql))
    {
        qDebug() << "创建报警索引失败:"
                 << query.lastError();

        return false;
    }

    return true;
}

int DatabaseManager::insertDevice(const DeviceConfig &config)
{
    // 设备 ID 作为应用内标识使用；复用最小空缺值，避免删除后只增不减。
    QSqlQuery idQuery(m_database);
    if (!idQuery.exec(R"(
        WITH RECURSIVE ids(candidate) AS (
            SELECT 1
            UNION ALL
            SELECT candidate + 1
            FROM ids
            WHERE candidate < COALESCE((SELECT MAX(device_id) FROM device), 0) + 1
        )
        SELECT candidate
        FROM ids
        WHERE NOT EXISTS (
            SELECT 1 FROM device WHERE device_id = candidate
        )
        ORDER BY candidate
        LIMIT 1
    )")) {
        qDebug() << "查找可用设备 ID 失败:" << idQuery.lastError();
        return -1;
    }
    if (!idQuery.next())
        return -1;
    const int deviceId = idQuery.value(0).toInt();

    QSqlQuery query(m_database);

    query.prepare(R"(
        INSERT INTO device
        (
            device_id,
            device_name,
            data_source,
            protocol_type,
            serial_port,
            baud_rate,
            data_bits,
            stop_bits,
            parity,
            rtu_slave_id,
            rtu_start_address,
            rtu_quantity,
            tcp_ip,
            tcp_port,
            tcp_unit_id,
            tcp_start_address,
            tcp_quantity
        )
        VALUES
        (
            :device_id,
            :device_name,
            :data_source,
            :protocol_type,
            :serial_port,
            :baud_rate,
            :data_bits,
            :stop_bits,
            :parity,
            :rtu_slave_id,
            :rtu_start_address,
            :rtu_quantity,
            :tcp_ip,
            :tcp_port,
            :tcp_unit_id,
            :tcp_start_address,
            :tcp_quantity
        )
    )");

    query.bindValue(":device_id", deviceId);

    query.bindValue(
        ":device_name",
        config.deviceName
        );

    query.bindValue(
        ":data_source",
        static_cast<int>(config.dataSource)
        );

    query.bindValue(
        ":protocol_type",
        static_cast<int>(config.protocolType)
        );

    query.bindValue(
        ":serial_port",
        config.serialPort
        );

    query.bindValue(
        ":baud_rate",
        config.baudRate
        );

    query.bindValue(
        ":data_bits",
        config.dataBits
        );

    query.bindValue(
        ":stop_bits",
        config.stopBits
        );

    query.bindValue(
        ":parity",
        config.parity
        );

    query.bindValue(
        ":rtu_slave_id",
        config.rtuSlaveId
        );

    query.bindValue(
        ":rtu_start_address",
        config.rtuStartAddress
        );

    query.bindValue(
        ":rtu_quantity",
        config.rtuQuantity
        );

    query.bindValue(
        ":tcp_ip",
        config.tcpIp
        );

    query.bindValue(
        ":tcp_port",
        config.tcpPort
        );

    query.bindValue(
        ":tcp_unit_id",
        config.tcpUnitId
        );

    query.bindValue(
        ":tcp_start_address",
        config.tcpStartAddress
        );

    query.bindValue(
        ":tcp_quantity",
        config.tcpQuantity
        );

    if (!query.exec())
    {
        qDebug()
        << "添加设备失败:"
        << query.lastError();

        return -1;
    }

    qDebug()
        << "设备添加成功:"
        << "ID =" << deviceId
        << "名称 =" << config.deviceName;

    return deviceId;
}

bool DatabaseManager::deleteDevice(int deviceId)
{
    if (!m_database.transaction()) {
        qDebug() << "开始删除设备事务失败:" << m_database.lastError();
        return false;
    }

    QSqlQuery historyQuery(m_database);
    historyQuery.prepare(R"(
        DELETE FROM device_history
        WHERE device_id = :device_id
    )");
    historyQuery.bindValue(":device_id", deviceId);
    if (!historyQuery.exec()) {
        qDebug() << "删除设备历史数据失败:" << historyQuery.lastError();
        m_database.rollback();
        return false;
    }

    QSqlQuery query(m_database);

    query.prepare(R"(
        DELETE FROM device
        WHERE device_id = :device_id
    )");

    query.bindValue(
        ":device_id",
        deviceId
        );

    if (!query.exec())
    {
        qDebug()
        << "删除设备失败:"
        << "ID =" << deviceId
        << query.lastError();

        m_database.rollback();

        return false;
    }

    if (query.numRowsAffected() == 0)
    {
        qDebug()
        << "删除设备失败:"
        << "设备不存在, ID =" << deviceId;

        m_database.rollback();

        return false;
    }

    if (!m_database.commit()) {
        qDebug() << "提交删除设备事务失败:" << m_database.lastError();
        m_database.rollback();
        return false;
    }

    qDebug()
        << "设备删除成功:"
        << "ID =" << deviceId;

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

void DatabaseManager::insertAlarm(const AlarmInfo &alarm)
{
    QSqlQuery query(m_database);

    query.prepare(
        "INSERT INTO alarm_history "
        "(device_id, alarm_type, message, recovered, timestamp) "
        "VALUES (?, ?, ?, ?, ?)"
        );

    query.addBindValue(alarm.deviceId);
    query.addBindValue(static_cast<int>(alarm.type));
    query.addBindValue(alarm.message);
    query.addBindValue(alarm.recovered ? 1 : 0);

    query.addBindValue(
        alarm.timestamp.toString("yyyy-MM-dd HH:mm:ss")
        );

    if (!query.exec())
    {
        qDebug() << "插入报警记录失败："
                 << query.lastError().text();
    }
}

void DatabaseManager::deleteAlarmHistory()
{
    QSqlQuery query(m_database);

    if (!query.exec("DELETE FROM alarm_history"))
    {
        qDebug() << "删除报警历史失败:"
                 << query.lastError().text();
    }
}
