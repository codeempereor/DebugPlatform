#ifndef DEBUGPLATFORM_DATACACHE_H
#define DEBUGPLATFORM_DATACACHE_H

/**
 * @file datacache.h
 * @brief 实时数据缓存
 *
 * 维护所有设备的实时寄存器数据。数据结构为：
 *     deviceId -> (registerAddr -> value)
 * 使用 QReadWriteLock 保护并发读写，支持多线程场景下的数据缓存与查询。
 */

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QReadWriteLock>

/**
 * @class DataCache
 * @brief 实时数据缓存管理器
 *
 * 为每个设备维护一份寄存器值映射，提供线程安全的读写接口。
 * 当值发生变化时发射 valueChanged 信号，UI 层可据此刷新显示。
 */
class DataCache : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DataCache(QObject *parent = nullptr);
    ~DataCache() override;

    /**
     * @brief 更新一个寄存器值
     * @param deviceId 设备 ID
     * @param registerAddr 寄存器地址
     * @param value 新值
     *
     * 若值与旧值不同则发射 valueChanged 信号。
     */
    void updateValue(const QString &deviceId, quint16 registerAddr, double value);

    /**
     * @brief 获取一个寄存器当前值
     * @param deviceId 设备 ID
     * @param registerAddr 寄存器地址
     * @return 当前值；若不存在则返回 0.0
     */
    double getValue(const QString &deviceId, quint16 registerAddr) const;

    /**
     * @brief 获取某设备的全部寄存器数据
     * @param deviceId 设备 ID
     * @return 寄存器地址到值的映射
     */
    QHash<quint16, double> getDeviceData(const QString &deviceId) const;

    /**
     * @brief 清空某设备的全部寄存器数据
     * @param deviceId 设备 ID
     */
    void clearDeviceData(const QString &deviceId);

    /**
     * @brief 获取当前已注册的设备列表
     * @return 设备 ID 列表
     */
    QStringList getDeviceList() const;

signals:
    /**
     * @brief 寄存器值变化信号
     * @param deviceId 设备 ID
     * @param registerAddr 寄存器地址
     * @param newValue 新值
     */
    void valueChanged(const QString &deviceId, quint16 registerAddr, double newValue);

    /**
     * @brief 设备数据被清空信号
     * @param deviceId 设备 ID
     */
    void deviceDataCleared(const QString &deviceId);

private:
    mutable QReadWriteLock m_lock;                              ///< 读写锁
    QHash<QString, QHash<quint16, double>> m_data;             ///< 数据存储: deviceId -> (addr -> value)
};

#endif // DEBUGPLATFORM_DATACACHE_H
