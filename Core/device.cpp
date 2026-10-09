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
    ++m_simulationTick;

    // 让模拟曲线连续波动，并周期性越界以便验证报警触发和恢复。
    const int temperaturePhase = m_simulationTick % 90;
    if (temperaturePhase >= 15 && temperaturePhase < 22)
        m_data.temperature = 52.0 + QRandomGenerator::global()->generateDouble() * 4.0;
    else if (temperaturePhase >= 55 && temperaturePhase < 62)
        m_data.temperature = 10.0 + QRandomGenerator::global()->generateDouble() * 3.0;
    else
        m_data.temperature = 25.0 + QRandomGenerator::global()->generateDouble() * 18.0;

    const int voltagePhase = m_simulationTick % 70;
    if (voltagePhase >= 28 && voltagePhase < 34)
        m_data.voltage = 232.0 + QRandomGenerator::global()->generateDouble() * 5.0;
    else if (voltagePhase >= 48 && voltagePhase < 54)
        m_data.voltage = 203.0 + QRandomGenerator::global()->generateDouble() * 5.0;
    else
        m_data.voltage = 214.0 + QRandomGenerator::global()->generateDouble() * 14.0;
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
