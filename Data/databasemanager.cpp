#include "databasemanager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <algorithm>


DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent){}

QString DatabaseManager::databasePath() const
{
    return m_database.databaseName();
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

QList<DeviceConfig> DatabaseManager::queryDevices()
{
    QList<DeviceConfig> devices;

    QSqlQuery query(m_database);

    query.prepare(R"(
        SELECT
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
        FROM device
        ORDER BY device_id ASC
    )");

    if (!query.exec())
    {
        qDebug()
        << "查询设备配置失败:"
        << query.lastError();

        return devices;
    }

    while (query.next())
    {
        DeviceConfig config;

        config.deviceId =
            query.value("device_id").toInt();

        config.deviceName =
            query.value("device_name").toString();

        config.dataSource =
            static_cast<DataSource>(
                query.value("data_source").toInt()
                );

        config.protocolType =
            static_cast<ProtocolType>(
                query.value("protocol_type").toInt()
                );

        config.serialPort =
            query.value("serial_port").toString();

        config.baudRate =
            query.value("baud_rate").toInt();

        config.dataBits =
            query.value("data_bits").toInt();

        config.stopBits =
            query.value("stop_bits").toInt();

        config.parity =
            query.value("parity").toString();

        config.rtuSlaveId =
            query.value("rtu_slave_id").toInt();

        config.rtuStartAddress =
            query.value("rtu_start_address").toInt();

        config.rtuQuantity =
            query.value("rtu_quantity").toInt();

        config.tcpIp =
            query.value("tcp_ip").toString();

        config.tcpPort =
            query.value("tcp_port").toInt();

        config.tcpUnitId =
            query.value("tcp_unit_id").toInt();

        config.tcpStartAddress =
            query.value("tcp_start_address").toInt();

        config.tcpQuantity =
            query.value("tcp_quantity").toInt();

        devices.append(config);
    }

    qDebug()
        << "读取设备配置成功:"
        << devices.size()
        << "个设备";

    return devices;
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

bool DatabaseManager::deviceExists(int deviceId)
{
    QSqlQuery query(m_database);

    query.prepare(R"(
        SELECT 1
        FROM device
        WHERE device_id = :device_id
        LIMIT 1
    )");

    query.bindValue(
        ":device_id",
        deviceId
        );

    if (!query.exec())
    {
        qDebug()
        << "检查设备是否存在失败:"
        << "ID =" << deviceId
        << query.lastError();

        return false;
    }

    return query.next();
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

QList<AlarmInfo> DatabaseManager::queryAlarmHistory()
{
    return queryAlarmHistory(500);
}

QList<AlarmInfo> DatabaseManager::queryAlarmHistory(int limit)
{
    QList<AlarmInfo> alarms;

    QSqlQuery query(m_database);

    query.prepare(
        "SELECT device_id, alarm_type, message, recovered, timestamp "
        "FROM alarm_history "
        "ORDER BY id DESC "
        "LIMIT :limit"
        );
    query.bindValue(":limit", qMax(1, limit));

    if (!query.exec())
    {
        qDebug() << "查询报警历史失败:"
                 << query.lastError().text();

        return alarms;
    }

    while (query.next())
    {
        AlarmInfo alarm;

        alarm.deviceId =
            query.value("device_id").toInt();

        alarm.type =
            static_cast<AlarmType>(
                query.value("alarm_type").toInt()
                );

        alarm.message =
            query.value("message").toString();

        alarm.recovered =
            query.value("recovered").toInt() != 0;

        alarm.timestamp =
            QDateTime::fromString(
                query.value("timestamp").toString(),
                "yyyy-MM-dd HH:mm:ss"
                );

        alarms.append(alarm);
    }

    std::reverse(alarms.begin(), alarms.end());

    return alarms;
}

QList<AlarmInfo> DatabaseManager::queryActiveAlarms()
{
    QList<AlarmInfo> alarms;

    QSqlQuery query(m_database);

    query.prepare(R"(
        SELECT a.device_id,
               a.alarm_type,
               a.message,
               a.recovered,
               a.timestamp
        FROM alarm_history a
        WHERE a.id = (
            SELECT b.id
            FROM alarm_history b
            WHERE b.device_id = a.device_id
              AND b.alarm_type = a.alarm_type
            ORDER BY b.id DESC
            LIMIT 1
        )
        AND a.recovered = 0
        ORDER BY a.timestamp ASC
    )");

    if (!query.exec())
    {
        qDebug() << "查询当前报警失败:"
                 << query.lastError().text();

        return alarms;
    }

    while (query.next())
    {
        AlarmInfo alarm;

        alarm.deviceId =
            query.value("device_id").toInt();

        alarm.type =
            static_cast<AlarmType>(
                query.value("alarm_type").toInt()
                );

        alarm.message =
            query.value("message").toString();

        alarm.recovered =
            query.value("recovered").toInt() != 0;

        alarm.timestamp =
            QDateTime::fromString(
                query.value("timestamp").toString(),
                "yyyy-MM-dd HH:mm:ss"
                );

        alarms.append(alarm);
    }

    return alarms;
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
