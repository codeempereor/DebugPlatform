#ifndef DEBUGPLATFORM_ALERTMANAGER_H
#define DEBUGPLATFORM_ALERTMANAGER_H

/**
 * @file alertmanager.h
 * @brief 告警管理器
 *
 * 为每个设备的寄存器配置上下限阈值，在数值越界时触发告警。
 * 告警历史在内存中维护，可按时间范围查询。
 */

#include <QObject>
#include <QString>
#include <QVector>
#include <QHash>
#include <QDateTime>
#include <QReadWriteLock>

/**
 * @struct AlertConfig
 * @brief 单个寄存器的告警配置
 */
struct AlertConfig
{
    quint16 registerAddr = 0;   ///< 寄存器地址
    double  minValue     = 0.0; ///< 下限
    double  maxValue     = 0.0; ///< 上限
    bool    enabled      = true; ///< 是否启用

    bool operator==(const AlertConfig &other) const
    {
        return registerAddr == other.registerAddr
            && qFuzzyCompare(minValue, other.minValue)
            && qFuzzyCompare(maxValue, other.maxValue)
            && enabled == other.enabled;
    }
};

/**
 * @struct Alert
 * @brief 一条告警记录
 */
struct Alert
{
    int         id           = 0;        ///< 告警 ID（自增）
    QString     deviceId;               ///< 设备 ID
    quint16     registerAddr = 0;       ///< 寄存器地址
    double      value        = 0.0;     ///< 触发时的值
    QString     level;                  ///< 级别字符串（WARNING / CRITICAL）
    QString     message;                ///< 告警描述
    QDateTime   timestamp;              ///< 时间戳
};

/**
 * @class AlertManager
 * @brief 告警管理器
 *
 * 通过 addAlertConfig() 为寄存器添加阈值配置，调用 checkValue()
 * 检查一个寄存器的值是否触发告警，若触发则发射 alertTriggered 信号。
 */
Q_DECLARE_METATYPE(Alert)

class AlertManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit AlertManager(QObject *parent = nullptr);
    ~AlertManager() override;

    /**
     * @brief 添加（或更新）一个寄存器的告警配置
     * @param deviceId 设备 ID
     * @param config 告警配置
     */
    void addAlertConfig(const QString &deviceId, const AlertConfig &config);

    /**
     * @brief 移除一个寄存器的告警配置
     * @param deviceId 设备 ID
     * @param registerAddr 寄存器地址
     */
    void removeAlertConfig(const QString &deviceId, quint16 registerAddr);

    /**
     * @brief 清空指定设备的所有告警配置
     */
    void clearDeviceConfigs(const QString &deviceId);

    /**
     * @brief 获取指定设备的全部告警配置（便于 UI 预填）
     */
    QHash<quint16, AlertConfig> allConfigsForDevice(const QString &deviceId) const;

    /**
     * @brief 检查一个寄存器值是否触发告警
     * @param deviceId 设备 ID
     * @param registerAddr 寄存器地址
     * @param value 要检查的值
     *
     * 若触发告警，将发射 alertTriggered 信号并记录到历史中。
     */
    void checkValue(const QString &deviceId, quint16 registerAddr, double value);

    /**
     * @brief 查询告警历史
     * @param deviceId 设备 ID
     * @param start 起始时间
     * @param end 结束时间
     * @return 告警列表（按时间升序）
     */
    QVector<Alert> getAlertHistory(const QString &deviceId,
                                   const QDateTime &start, const QDateTime &end);

signals:
    /**
     * @brief 告警触发信号
     * @param alert 告警详情
     */
    void alertTriggered(const Alert &alert);

private:
    /**
     * @brief 内部查找指定配置
     * @param deviceId 设备 ID
     * @param registerAddr 寄存器地址
     * @param outConfig 输出配置（若找到）
     * @return true 表示找到
     */
    bool findConfigLocked(const QString &deviceId, quint16 registerAddr,
                          AlertConfig &outConfig) const;

    mutable QReadWriteLock                      m_lock;          ///< 读写锁
    QHash<QString, QHash<quint16, AlertConfig>> m_configs;       ///< 配置: deviceId -> (addr -> config)
    QVector<Alert>                              m_history;       ///< 告警历史
    int                                         m_nextAlertId = 1; ///< 下一个告警 ID
};

#endif // DEBUGPLATFORM_ALERTMANAGER_H
