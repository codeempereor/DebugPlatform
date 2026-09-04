#ifndef DEBUGPLATFORM_SERVICELOCATOR_H
#define DEBUGPLATFORM_SERVICELOCATOR_H

/**
 * @file servicelocator.h
 * @brief 服务定位器（单例）
 *
 * 统一创建、管理和访问应用中的所有服务组件。
 * 通过 ServiceLocator 可以获取各服务实例，并在初始化时
 * 自动连接服务间的信号槽，建立数据流管道。
 *
 * 服务列表：
 *   - DataCache      实时数据缓存
 *   - DataPersistent 数据持久化（SQLite）
 *   - AlertManager   告警管理器
 *   - LogManager     日志管理器
 *   - DeviceManager  设备管理器
 *   - PlaybackEngine 报文回放引擎
 */

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QList>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include "config.h"
#include "data/datapersistent.h"

class DataCache;
class AlertManager;
class LogManager;
class DeviceManager;
class PlaybackEngine;
class BinaryFrameParser;
class ModbusRtuProtocol; // 静态工具类，仅作前向引用占位

struct Alert;

/**
 * @struct PendingRegisterData
 * @brief 待批量写入的寄存器数据
 */
struct PendingRegisterData
{
    QString deviceId;           ///< 设备 ID
    quint16 registerAddr = 0;   ///< 寄存器地址
    double value = 0.0;         ///< 数值
};

/**
 * @class ServiceLocator
 * @brief 服务定位器（单例）
 *
 * 使用 SINGLETON 宏实现单例模式。在应用启动时调用 initialize()
 * 创建所有服务并连接信号槽；在应用退出时调用 shutdown() 释放资源。
 */
class ServiceLocator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 初始化所有服务
     * @param parent 父对象（通常为 QApplication 实例）
     *
     * 创建所有服务实例并连接服务间的信号槽：
     *   - DeviceManager::deviceDataReceived → DataCache::updateValue
     *   - DataCache::valueChanged → AlertManager::checkValue
     *   - AlertManager::alertTriggered → DataPersistent::insertAlert
     *   - DataCache::valueChanged → DataPersistent::insertRegisterData
     *   - LogManager::logMessage → 日志输出
     */
    void initialize(QObject *parent = nullptr);

    ~ServiceLocator();

    /**
     * @brief 关闭所有服务
     *
     * 断开信号槽连接并释放所有服务实例。
     */
    void shutdown();

    /**
     * @brief 获取实时数据缓存
     * @return DataCache 实例指针
     */
    DataCache *dataCache() const;

    /**
     * @brief 获取数据持久化管理器
     * @return DataPersistent 实例指针
     */
    DataPersistent *dataPersistent() const;

    /**
     * @brief 获取告警管理器
     * @return AlertManager 实例指针
     */
    AlertManager *alertManager() const;

    /**
     * @brief 获取日志管理器
     * @return LogManager 实例指针
     */
    LogManager *logManager() const;

    /**
     * @brief 获取设备管理器
     * @return DeviceManager 实例指针
     */
    DeviceManager *deviceManager() const;

    /**
     * @brief 获取报文回放引擎
     * @return PlaybackEngine 实例指针
     */
    PlaybackEngine *playbackEngine() const;

    /**
     * @brief 标记一次命令发送，用于对端响应到达后计算响应时间。
     *
     * 若同一设备在 timeoutMs （默认 3000ms）内重复发送，将覆盖前一条。
     */
    void markSendTimestamp(const QString &deviceId, const QByteArray &txFrame = {}, int timeoutMs = 3000);

signals:
    /**
     * @brief 原始设备数据接收（转发给 UI 显示原始 hex/byte）
     */
    void deviceRawDataReceived(const QString &deviceId, const QByteArray &data);

    /**
     * @brief 寄存器值更新（经协议解析后）
     *
     * 由 ServiceLocator 在解析完 Modbus / 二进制帧后发射，
     * UI 层直接监听此信号以刷新数据监控表格。
     */
    void deviceRegisterUpdated(const QString &deviceId, quint16 addr, double value,
                               const QString &name, const QString &unit);

    /**
     * @brief Modbus 响应回显（用于命令面板显示响应和耗时）
     */
    void modbusResponseReceived(const QString &deviceId,
                                const QString &txHex,
                                const QString &rxHex,
                                int responseTimeMs);

    /**
     * @brief 单次命令响应超时（用于命令面板显示"超时"）。
     * @param deviceId 设备 ID
     * @param txHex  发送帧 HEX
     */
    void modbusRequestTimedOut(const QString &deviceId, const QString &txHex);

    /**
     * @brief 阈值告警触发（广播给 MainWindow 做状态栏计数 / 颜色）。
     */
    void alertOccurred(const QString &deviceId, quint16 registerAddr, double value,
                       const QString &level, const QString &message,
                       const QDateTime &timestamp);

private:
    /**
     * @brief 检查是否已初始化
     * @return true 表示已初始化
     */
    bool isInitialized() const;

    QPointer<DataCache>      m_dataCache;       ///< 实时数据缓存
    QPointer<DataPersistent> m_dataPersistent;   ///< 数据持久化
    QPointer<AlertManager>   m_alertManager;    ///< 告警管理器
    QPointer<LogManager>     m_logManager;      ///< 日志管理器
    QPointer<DeviceManager>  m_deviceManager;   ///< 设备管理器
    QPointer<PlaybackEngine> m_playbackEngine;  ///< 回放引擎
    bool                     m_initialized = false; ///< 初始化标志

    // 批量节流相关成员
    QTimer                  *m_batchTimer = nullptr;  ///< 批量写入定时器
    QList<PendingRegisterData> m_pendingRegisterData; ///< 待写入数据队列

    // 协议解析：每设备一个二进制帧解析器（粘包/半包状态机需要按设备隔离）
    QHash<QString, BinaryFrameParser*>  m_binaryParsers;

    // Modbus TCP 接收缓冲区：处理粘包/半包
    QHash<QString, QByteArray> m_tcpBuffers;

    // 命令响应时间测量：deviceId -> (sendTsMs, txHex, timeoutMs)
    struct PendingSend {
        qint64    sendTsMs    = 0;
        QByteArray txFrame;
        int       timeoutMs   = 3000;
    };
    QHash<QString, PendingSend> m_pendingSends;

    SINGLETON(ServiceLocator)
};

#endif // DEBUGPLATFORM_SERVICELOCATOR_H