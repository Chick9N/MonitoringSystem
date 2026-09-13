#include "devicemanager.h"

#include <QRandomGenerator>
#include <QDebug>
DeviceManager::DeviceManager(QObject *parent) : QObject(parent) {

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

QList<Device*> DeviceManager::devices() const{
    return m_devices;
}

void DeviceManager::updateAllDevices(){
    for(Device* device:m_devices){
        device->updateData();
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
