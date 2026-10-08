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
