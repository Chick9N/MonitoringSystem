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

DeviceData Device::data() const{
    return m_data;
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

bool Device::isRunning() const
{
    return m_running;
}
