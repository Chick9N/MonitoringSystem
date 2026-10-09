#ifndef DATABASEWRITEQUEUE_H
#define DATABASEWRITEQUEUE_H

#include <QObject>
#include <QThread>
#include <QString>
#include <memory>
#include "../Core/devicedata.h"
#include "../Core/alarm.h"

class DatabaseWriterState;

class DatabaseWriteQueue : public QObject
{
public:
    explicit DatabaseWriteQueue(const QString &databasePath,
                                QObject *parent = nullptr);
    ~DatabaseWriteQueue() override;

    void enqueueDeviceData(int deviceId, const DeviceData &data);
    void enqueueAlarm(const AlarmInfo &alarm);
    void enqueueAlarmAcknowledgement(int deviceId, AlarmType type);

private:
    QThread *m_thread = nullptr;
    QObject *m_workerContext = nullptr;
    std::shared_ptr<DatabaseWriterState> m_state;
};

#endif // DATABASEWRITEQUEUE_H
