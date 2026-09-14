#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QList>
#include "Device.h"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    void addDevice(Device *device);
    Device* getDevice(int deviceId);
    QList<Device*> devices() const;

    void startAll();
    void stopAll();

public slots:
    void updateAllDevices();

signals:
    void deviceDataUpdated(int deviceId,const DeviceData &data);

private:
    QList<Device*> m_devices; // 设备容器
};

#endif // DEVICEMANAGER_H
