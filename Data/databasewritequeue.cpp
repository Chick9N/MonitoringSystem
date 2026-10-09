#include "databasewritequeue.h"

#include "databasemanager.h"
#include <QMetaObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>

class DatabaseWriterState
{
public:
    QSqlDatabase database;
    QString connectionName;
};

DatabaseWriteQueue::DatabaseWriteQueue(const QString &databasePath,
                                       QObject *parent)
    : QObject(parent)
    , m_thread(new QThread(this))
    , m_workerContext(new QObject)
    , m_state(std::make_shared<DatabaseWriterState>())
{
    m_workerContext->moveToThread(m_thread);
    connect(m_thread, &QThread::finished,
            m_workerContext, &QObject::deleteLater);
    m_thread->start();

    const auto state = m_state;
    QMetaObject::invokeMethod(m_workerContext, [state, databasePath]() {
        state->connectionName = QStringLiteral("device_writer_%1")
            .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
        state->database = QSqlDatabase::addDatabase("QSQLITE", state->connectionName);
        state->database.setDatabaseName(databasePath);
        if (!state->database.open()) {
            qWarning() << "数据库写入线程连接失败:"
                       << state->database.lastError();
            return;
        }

        QSqlQuery query(state->database);
        query.exec("PRAGMA busy_timeout = 5000");
    }, Qt::QueuedConnection);
}

DatabaseWriteQueue::~DatabaseWriteQueue()
{
    if (!m_thread || !m_thread->isRunning())
        return;

    const auto state = m_state;
    QMetaObject::invokeMethod(m_workerContext, [state]() {
        if (state->database.isValid()) {
            state->database.close();
            state->database = QSqlDatabase();
            QSqlDatabase::removeDatabase(state->connectionName);
        }
    }, Qt::BlockingQueuedConnection);

    m_thread->quit();
    m_thread->wait();
}

void DatabaseWriteQueue::enqueueDeviceData(int deviceId,
                                           const DeviceData &data)
{
    const auto state = m_state;
    QMetaObject::invokeMethod(m_workerContext,
        [state, deviceId, data]() {
            if (!state->database.isOpen())
                return;
            if (!DatabaseManager::insertDeviceData(
                    state->database, deviceId, data)) {
                qWarning() << "异步写入设备历史数据失败:" << deviceId;
            }
        }, Qt::QueuedConnection);
}

void DatabaseWriteQueue::enqueueAlarm(const AlarmInfo &alarm)
{
    const auto state = m_state;
    QMetaObject::invokeMethod(m_workerContext, [state, alarm]() {
        if (!state->database.isOpen())
            return;
        QSqlQuery query(state->database);
        query.prepare(R"(
            INSERT INTO alarm_history
                (device_id, alarm_type, message, recovered, timestamp, acknowledged)
            VALUES (:device_id, :alarm_type, :message, :recovered, :timestamp, :acknowledged)
        )");
        query.bindValue(":device_id", alarm.deviceId);
        query.bindValue(":alarm_type", static_cast<int>(alarm.type));
        query.bindValue(":message", alarm.message);
        query.bindValue(":recovered", alarm.recovered ? 1 : 0);
        query.bindValue(":timestamp", alarm.timestamp.toString("yyyy-MM-dd HH:mm:ss"));
        query.bindValue(":acknowledged", alarm.acknowledged ? 1 : 0);
        if (!query.exec())
            qWarning() << "异步写入报警记录失败:" << query.lastError();
    }, Qt::QueuedConnection);
}

void DatabaseWriteQueue::enqueueAlarmAcknowledgement(int deviceId, AlarmType type)
{
    const auto state = m_state;
    QMetaObject::invokeMethod(m_workerContext, [state, deviceId, type]() {
        if (!state->database.isOpen())
            return;
        QSqlQuery query(state->database);
        query.prepare(R"(
            UPDATE alarm_history SET acknowledged = 1
            WHERE id = (
                SELECT MAX(id) FROM alarm_history
                WHERE device_id = :device_id AND alarm_type = :alarm_type AND recovered = 0
            )
        )");
        query.bindValue(":device_id", deviceId);
        query.bindValue(":alarm_type", static_cast<int>(type));
        if (!query.exec())
            qWarning() << "报警确认状态写入失败:" << query.lastError();
    }, Qt::QueuedConnection);
}
