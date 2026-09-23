#include "devicemanager.h"

#include <QRandomGenerator>
#include <QDebug>
DeviceManager::DeviceManager(QObject *parent)
    : QObject(parent)
    , m_alarmManager(new AlarmManager(this))
{
    // 转发信号
    connect(
        m_alarmManager,
        &AlarmManager::alarmTriggered,
        this,
        &DeviceManager::alarmTriggered
    );
}

void DeviceManager::addDevice(Device *device){
    if(!device)
        return;

    m_devices.append(device);

    connect(device,
            &Device::dataUpdated,
            this,
            [this,device](const DeviceData &data){
                emit deviceDataUpdated(device->id(),data);
            });
}

Device* DeviceManager::getDevice(int deviceId){
    if(deviceId == -1) return nullptr;
    for(Device* device:m_devices){
        if(device->id() == deviceId)
            return device;
    }
    return nullptr;
}

QList<Device*> DeviceManager::devices() const{
    return m_devices;
}

void DeviceManager::updateAllDevices(){
    for(Device* device:m_devices){
        device->updateData();

        m_alarmManager->checkDeviceData(
            device->id(),
            device->data()
        );
    }
}

void DeviceManager::startAll(){
    for(Device* device:m_devices){
        device->start();
    }
}


void DeviceManager::stopAll(){
    for(Device* device:m_devices){
        device->stop();
    }
}
