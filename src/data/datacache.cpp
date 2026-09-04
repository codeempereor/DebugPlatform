#include "datacache.h"

/**
 * @file datacache.cpp
 * @brief 实时数据缓存实现
 */

DataCache::DataCache(QObject *parent)
    : QObject(parent)
{
}

DataCache::~DataCache() = default;

void DataCache::updateValue(const QString &deviceId, quint16 registerAddr, double value)
{
    bool changed = false;
    {
        QWriteLocker locker(&m_lock);
        auto &deviceMap = m_data[deviceId];
        auto it = deviceMap.find(registerAddr);
        if (it == deviceMap.end()) {
            deviceMap.insert(registerAddr, value);
            changed = true;
        } else if (!qFuzzyCompare(it.value(), value)) {
            it.value() = value;
            changed = true;
        }
    }

    if (changed) {
        emit valueChanged(deviceId, registerAddr, value);
    }
}

double DataCache::getValue(const QString &deviceId, quint16 registerAddr) const
{
    QReadLocker locker(&m_lock);
    const auto deviceMap = m_data.constFind(deviceId);
    if (deviceMap == m_data.constEnd()) {
        return 0.0;
    }
    const auto value = deviceMap.value().constFind(registerAddr);
    if (value == deviceMap.value().constEnd()) {
        return 0.0;
    }
    return value.value();
}

QHash<quint16, double> DataCache::getDeviceData(const QString &deviceId) const
{
    QReadLocker locker(&m_lock);
    const auto deviceMap = m_data.constFind(deviceId);
    if (deviceMap == m_data.constEnd()) {
        return {};
    }
    return deviceMap.value();
}

void DataCache::clearDeviceData(const QString &deviceId)
{
    {
        QWriteLocker locker(&m_lock);
        m_data.remove(deviceId);
    }
    emit deviceDataCleared(deviceId);
}

QStringList DataCache::getDeviceList() const
{
    QReadLocker locker(&m_lock);
    return m_data.keys();
}
