#include "alertmanager.h"

#include <algorithm>

/**
 * @file alertmanager.cpp
 * @brief 告警管理器实现
 */

AlertManager::AlertManager(QObject *parent)
    : QObject(parent)
{
}

AlertManager::~AlertManager() = default;

void AlertManager::addAlertConfig(const QString &deviceId, const AlertConfig &config)
{
    QWriteLocker locker(&m_lock);
    m_configs[deviceId][config.registerAddr] = config;
}

void AlertManager::removeAlertConfig(const QString &deviceId, quint16 registerAddr)
{
    QWriteLocker locker(&m_lock);
    auto deviceIt = m_configs.find(deviceId);
    if (deviceIt == m_configs.end()) {
        return;
    }
    deviceIt.value().remove(registerAddr);
    if (deviceIt.value().isEmpty()) {
        m_configs.erase(deviceIt);
    }
}

void AlertManager::clearDeviceConfigs(const QString &deviceId)
{
    QWriteLocker locker(&m_lock);
    m_configs.remove(deviceId);
}

QHash<quint16, AlertConfig> AlertManager::allConfigsForDevice(const QString &deviceId) const
{
    QReadLocker locker(&m_lock);
    auto it = m_configs.constFind(deviceId);
    if (it == m_configs.constEnd()) return {};
    return it.value();
}

bool AlertManager::findConfigLocked(const QString &deviceId, quint16 registerAddr,
                                     AlertConfig &outConfig) const
{
    auto deviceIt = m_configs.constFind(deviceId);
    if (deviceIt == m_configs.constEnd()) {
        return false;
    }
    auto addrIt = deviceIt.value().constFind(registerAddr);
    if (addrIt == deviceIt.value().constEnd()) {
        return false;
    }
    outConfig = addrIt.value();
    return true;
}

void AlertManager::checkValue(const QString &deviceId, quint16 registerAddr, double value)
{
    AlertConfig config;
    {
        QReadLocker locker(&m_lock);
        if (!findConfigLocked(deviceId, registerAddr, config)) {
            return;
        }
    }

    if (!config.enabled) {
        return;
    }

    QString level;
    QString message;

    if (value < config.minValue) {
        level = QStringLiteral("WARNING");
        message = QStringLiteral("寄存器 0x%1 低值告警: 当前值 %2 < 下限 %3")
                      .arg(QString::number(registerAddr, 16).toUpper())
                      .arg(value, 0, 'f', 3)
                      .arg(config.minValue, 0, 'f', 3);
    } else if (value > config.maxValue) {
        level = QStringLiteral("WARNING");
        message = QStringLiteral("寄存器 0x%1 高值告警: 当前值 %2 > 上限 %3")
                      .arg(QString::number(registerAddr, 16).toUpper())
                      .arg(value, 0, 'f', 3)
                      .arg(config.maxValue, 0, 'f', 3);
    } else {
        return; // 未触发
    }

    Alert alert;
    {
        QWriteLocker locker(&m_lock);
        alert.id           = m_nextAlertId++;
        alert.deviceId     = deviceId;
        alert.registerAddr = registerAddr;
        alert.value        = value;
        alert.level        = level;
        alert.message      = message;
        alert.timestamp    = QDateTime::currentDateTime();
        m_history.append(alert);
    }

    emit alertTriggered(alert);
}

QVector<Alert> AlertManager::getAlertHistory(const QString &deviceId,
                                              const QDateTime &start, const QDateTime &end)
{
    QReadLocker locker(&m_lock);
    QVector<Alert> result;
    result.reserve(m_history.size());

    for (const Alert &alert : m_history) {
        if (!deviceId.isEmpty() && alert.deviceId != deviceId) {
            continue;
        }
        if (alert.timestamp < start || alert.timestamp > end) {
            continue;
        }
        result.append(alert);
    }

    // 时间升序排序
    std::sort(result.begin(), result.end(),
              [](const Alert &a, const Alert &b) {
                  return a.timestamp < b.timestamp;
              });

    return result;
}
