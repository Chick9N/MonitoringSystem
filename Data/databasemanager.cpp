#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QSet>
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
                           tcp_ip, tcp_port, tcp_unit_id, tcp_start_address, tcp_quantity,
                           temperature_low, temperature_high, voltage_low, voltage_high
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
                        config.temperatureLow = query.value(17).toDouble();
                        config.temperatureHigh = query.value(18).toDouble();
                        config.voltageLow = query.value(19).toDouble();
                        config.voltageHigh = query.value(20).toDouble();
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
                    alarm.acknowledged = query.value(5).toBool();
                    alarms.append(alarm);
                }
            };

            readAlarms(R"(
                SELECT device_id, alarm_type, message, recovered, timestamp, acknowledged
                FROM alarm_history
                ORDER BY id DESC
                LIMIT :limit
            )", snapshot.alarmHistory);
            std::reverse(snapshot.alarmHistory.begin(), snapshot.alarmHistory.end());

            readAlarms(R"(
                SELECT a.device_id, a.alarm_type, a.message, a.recovered, a.timestamp, a.acknowledged
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

QList<DeviceHistory> DatabaseManager::loadDeviceHistorySnapshot(
    const QString &databasePath,
    int deviceId,
    const QDateTime &startTime,
    const QDateTime &endTime)
{
    QList<DeviceHistory> result;
    const QString connectionName = QStringLiteral("history_read_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(databasePath);
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
        if (db.open()) {
            QSqlQuery query(db);
            query.prepare(R"(
                SELECT id, device_id, temperature, voltage, online, timestamp
                FROM device_history
                WHERE device_id = :device_id
                  AND timestamp >= :start_time
                  AND timestamp <= :end_time
                ORDER BY timestamp ASC
            )");
            query.bindValue(":device_id", deviceId);
            query.bindValue(":start_time", startTime.toString("yyyy-MM-dd HH:mm:ss"));
            query.bindValue(":end_time", endTime.toString("yyyy-MM-dd HH:mm:ss"));
            if (query.exec()) {
                while (query.next()) {
                    DeviceHistory item;
                    item.id = query.value(0).toInt();
                    item.deviceId = query.value(1).toInt();
                    item.data.temperature = query.value(2).toDouble();
                    item.data.voltage = query.value(3).toDouble();
                    item.data.isOnline = query.value(4).toBool();
                    item.timestamp = QDateTime::fromString(
                        query.value(5).toString(), "yyyy-MM-dd HH:mm:ss");
                    result.append(item);
                }
            } else {
                qWarning() << "后台查询设备历史失败:" << query.lastError();
            }
        } else {
            qWarning() << "后台历史查询连接失败:" << db.lastError();
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return result;
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
            device_id INTEGER PRIMARY KEY,
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
            tcp_quantity INTEGER,
            temperature_low REAL NOT NULL DEFAULT 15.0,
            temperature_high REAL NOT NULL DEFAULT 50.0,
            voltage_low REAL NOT NULL DEFAULT 210.0,
            voltage_high REAL NOT NULL DEFAULT 230.0
        )
    )";

    if (!query.exec(createDeviceTableSql))
    {
        qDebug() << "创建设备配置表失败:"
                 << query.lastError();

        return false;
    }

    // 对已有数据库执行兼容迁移；旧设备采用安全默认报警阈值。
    const QList<QPair<QString, QString>> deviceMigrations = {
        {QStringLiteral("temperature_low"), QStringLiteral("REAL NOT NULL DEFAULT 15.0")},
        {QStringLiteral("temperature_high"), QStringLiteral("REAL NOT NULL DEFAULT 50.0")},
        {QStringLiteral("voltage_low"), QStringLiteral("REAL NOT NULL DEFAULT 210.0")},
        {QStringLiteral("voltage_high"), QStringLiteral("REAL NOT NULL DEFAULT 230.0")}
    };
    QSet<QString> existingColumns;
    if (query.exec("PRAGMA table_info(device)")) {
        while (query.next())
            existingColumns.insert(query.value("name").toString());
    }
    for (const auto &migration : deviceMigrations) {
        if (existingColumns.contains(migration.first))
            continue;
        if (!query.exec(QStringLiteral("ALTER TABLE device ADD COLUMN %1 %2")
                            .arg(migration.first, migration.second))) {
            qWarning() << "设备表迁移失败:" << migration.first << query.lastError();
            return false;
        }
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
            timestamp DATETIME NOT NULL,
            acknowledged INTEGER NOT NULL DEFAULT 0
        )
    )";

    if (!query.exec(createAlarmTableSql))
    {
        qDebug() << "创建报警历史表失败:"
                 << query.lastError();

        return false;
    }

    QSet<QString> alarmColumns;
    if (query.exec("PRAGMA table_info(alarm_history)")) {
        while (query.next())
            alarmColumns.insert(query.value("name").toString());
    }
    if (!alarmColumns.contains(QStringLiteral("acknowledged")) &&
        !query.exec("ALTER TABLE alarm_history ADD COLUMN acknowledged INTEGER NOT NULL DEFAULT 0")) {
        qWarning() << "报警表确认状态迁移失败:" << query.lastError();
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
    // 删除设备会清理历史，因此可安全复用最小空缺 ID。
    QSqlQuery idQuery(m_database);
    if (!idQuery.exec(R"(
        WITH RECURSIVE ids(candidate) AS (
            SELECT 1
            UNION ALL
            SELECT candidate + 1 FROM ids
            WHERE candidate < COALESCE((SELECT MAX(device_id) FROM device), 0) + 1
        )
        SELECT candidate FROM ids
        WHERE NOT EXISTS (SELECT 1 FROM device WHERE device_id = candidate)
        ORDER BY candidate LIMIT 1
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
            tcp_quantity,
            temperature_low,
            temperature_high,
            voltage_low,
            voltage_high
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
            :tcp_quantity,
            :temperature_low,
            :temperature_high,
            :voltage_low,
            :voltage_high
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

    query.bindValue(":temperature_low", config.temperatureLow);
    query.bindValue(":temperature_high", config.temperatureHigh);
    query.bindValue(":voltage_low", config.voltageLow);
    query.bindValue(":voltage_high", config.voltageHigh);

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

    // 同一事务中清理历史采样和报警记录，避免删除后的悬挂数据。
    QSqlQuery historyQuery(m_database);
    historyQuery.prepare("DELETE FROM device_history WHERE device_id = :device_id");
    historyQuery.bindValue(":device_id", deviceId);
    if (!historyQuery.exec()) {
        qDebug() << "删除设备历史数据失败:" << historyQuery.lastError();
        m_database.rollback();
        return false;
    }

    QSqlQuery alarmQuery(m_database);
    alarmQuery.prepare("DELETE FROM alarm_history WHERE device_id = :device_id");
    alarmQuery.bindValue(":device_id", deviceId);
    if (!alarmQuery.exec()) {
        qDebug() << "删除设备报警历史失败:" << alarmQuery.lastError();
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

bool DatabaseManager::insertDeviceData(
    QSqlDatabase &database,
    int deviceId,
    const DeviceData &data)
{
    QSqlQuery query(database);

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
