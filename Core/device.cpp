#include "device.h"

#include <QRandomGenerator>
#include <QDebug>
Device::Device(int id,QObject *parent):QObject(parent),m_id(id) {
    m_data.temperature = 25.0;
    m_data.voltage = 220.0;
    m_data.isOnline = true;
}

int Device::id() const{
    return m_id;
}

const DeviceData& Device::data() const{
    return m_data;
}

void Device::setConfig(const DeviceConfig &config)
{
    m_config = config;
    m_id = config.deviceId;
}

const DeviceConfig& Device::config() const
{
    return m_config;
}

void Device::updateData(){
    if (!m_running)
    {
        m_data.isOnline = false;
        emit dataUpdated(m_data);
        return;
    }

    m_data.isOnline = true;
    m_data.temperature = 20.0 + QRandomGenerator::global()->generateDouble()*40.0;
    m_data.voltage = 220.0 + QRandomGenerator::global()->generateDouble()*3.0;
    emit dataUpdated(m_data);
}

void Device::setData(const DeviceData &data)
{
    m_data = data;

    emit dataUpdated(m_data);
}

void Device::start()
{
    m_running = true;
    m_data.isOnline = true;

    emit dataUpdated(m_data);
}

void Device::stop()
{
    m_running = false;
    m_data.isOnline = false;

    emit dataUpdated(m_data);
}
