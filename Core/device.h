#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include "devicedata.h"

class Device : public QObject
{
    Q_OBJECT
public:
    explicit Device(int id,QObject *parent = nullptr);
    int id() const;
    const DeviceData& data() const;
    void updateData();
    void setData(const DeviceData &data);

    void start();
    void stop();
    bool isRunning() const;
signals:
    void dataUpdated(const DeviceData &data);

private:
    int m_id;
    DeviceData m_data;
    bool m_running = false;
};

#endif // DEVICE_H
