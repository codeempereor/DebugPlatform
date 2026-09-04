#ifndef DEBUGPLATFORM_DATAPERSISTENT_H
#define DEBUGPLATFORM_DATAPERSISTENT_H

/**
 * @file datapersistent.h
 * @brief SQLite 数据持久化管理
 *
 * 负责将设备、寄存器数据、告警、配置等信息持久化到 SQLite 数据库。
 * 使用 QSqlDatabase 进行单线程同步操作，支持批量事务优化。
 *
 * 数据表结构：
 *   - devices        设备信息
 *   - register_data  寄存器历史数据
 *   - alerts         告警记录
 *   - configs        键值对配置
 */

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QDateTime>
#include <QPair>
#include <QSqlDatabase>

/**
 * @class DataPersistent
 * @brief SQLite 数据持久化管理类
 *
 * 所有方法均为同步调用，调用线程需具备事件循环以保证 QtSql 正常工作。
 * 在 initialize() 成功之前，其它读写方法将直接返回失败。
 */
class DataPersistent : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DataPersistent(QObject *parent = nullptr);
    ~DataPersistent() override;

    /**
     * @brief 初始化数据库
     * @param dbPath 数据库文件路径
     * @return true 表示初始化成功
     *
     * 会创建所需全部数据表与索引（若不存在）。
     */
    bool initialize(const QString &dbPath);

    /**
     * @brief 添加设备
     * @param id 设备 ID
     * @param name 设备名称
     * @param type 设备类型
     * @param config 设备配置（JSON 字符串）
     * @return true 表示成功
     */
    bool addDevice(const QString &id, const QString &name,
                   const QString &type, const QString &config);

    /**
     * @brief 删除设备及其所有关联数据
     * @param id 设备 ID
     * @return true 表示成功
     */
    bool removeDevice(const QString &id);

    /**
     * @brief 插入寄存器数据
     * @param deviceId 设备 ID
     * @param addr 寄存器地址
     * @param value 数值
     * @return true 表示成功
     */
    bool insertRegisterData(const QString &deviceId, quint16 addr, double value);

    /**
     * @brief 批量插入寄存器数据（事务包裹，性能远高于逐条 INSERT）
     * @param batch 批量数据；若为空直接返回 false
     * @return true 表示所有数据均写入成功
     */
    bool insertRegisterDataBatch(const QVector<QPair<QString, QPair<quint16, double>>> &batch);

    /**
     * @brief 加载数据库中所有设备配置
     * @return 每项 (id, name, type, config) 的列表
     */
    QVector<QVariantMap> loadAllDevices();

    /**
     * @brief 插入告警记录
     * @param deviceId 设备 ID
     * @param addr 寄存器地址
     * @param value 触发值
     * @param level 告警级别（"WARNING" / "CRITICAL" 等）
     * @param message 告警描述
     * @return true 表示成功
     */
    bool insertAlert(const QString &deviceId, quint16 addr, double value,
                     const QString &level, const QString &message);

    /**
     * @brief 查询寄存器历史数据
     * @param deviceId 设备 ID
     * @param addr 寄存器地址
     * @param start 起始时间
     * @param end 结束时间
     * @return 查询结果列表，每条记录包含 device_id / register_addr / value / timestamp
     */
    QVector<QVariantMap> queryRegisterData(const QString &deviceId, quint16 addr,
                                            const QDateTime &start, const QDateTime &end);

    /**
     * @brief 查询告警历史
     * @param deviceId 设备 ID
     * @param start 起始时间
     * @param end 结束时间
     * @return 查询结果列表
     */
    QVector<QVariantMap> queryAlerts(const QString &deviceId,
                                     const QDateTime &start, const QDateTime &end);

    /**
     * @brief 查询最近 N 条告警（按 id 倒序）
     * @param limit 返回条数上限
     * @return 每行字段同 queryAlerts
     */
    QVector<QVariantMap> queryRecentAlerts(int limit = 10);

    /**
     * @brief 保存配置项（setConfig 别名，便于业务层一致命名）
     */
    bool setConfig(const QString &key, const QString &value) { return saveConfig(key, value); }

    /**
     * @brief 加载配置项（getConfig 别名，便于业务层一致命名）
     */
    QString getConfig(const QString &key) { return loadConfig(key); }

    /**
     * @brief 访问底层数据库连接（用于临时查询/UI 展示）
     */
    QSqlDatabase database() const { return m_db; }

    /**
     * @brief 保存配置项
     * @param key 配置键
     * @param value 配置值
     * @return true 表示成功
     */
    bool saveConfig(const QString &key, const QString &value);

    /**
     * @brief 加载配置项
     * @param key 配置键
     * @return 配置值；若不存在则返回空字符串
     */
    QString loadConfig(const QString &key);

    /**
     * @brief 清理过期数据
     * @param daysRetention 保留天数，早于此时间的数据将被删除
     */
    void cleanupOldData(int daysRetention);

    /**
     * @brief 检查数据库连接是否已就绪（已初始化+已open）
     * @return true 表示就绪
     */
    bool isReady() const;

private:
    /**
     * @brief 建表及索引
     * @return true 表示成功
     */
    bool createTables();

    /**
     * @brief 在调试模式下打印查询执行计划
     * @param sql 查询语句
     */
    void explainQuery(const QString &sql) const;

    QSqlDatabase m_db;      ///< 数据库连接
    QString      m_connectionName; ///< 当前连接名（析构时按名释放）
    bool         m_initialized = false; ///< 是否已初始化
};

#endif // DEBUGPLATFORM_DATAPERSISTENT_H
