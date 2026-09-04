#ifndef DEBUGPLATFORM_RECONNECTCONTROLLER_H
#define DEBUGPLATFORM_RECONNECTCONTROLLER_H

/**
 * @file reconnectcontroller.h
 * @brief 重连控制器
 *
 * 为单个设备提供基于指数退避算法的自动重连能力。
 * 状态机驱动：IDLE -> CONNECTING -> CONNECTED -> DISCONNECTED -> RECONNECTING。
 */

#include <QObject>
#include <QPointer>
#include <QTimer>
#include "core/config.h"
#include "device/deviceconfig.h"

class ICommunication;

/**
 * @enum ReconnectState
 * @brief 重连状态机状态
 */
enum class ReconnectState {
    IDLE,          ///< 空闲（未启动）
    CONNECTING,    ///< 首次连接中
    CONNECTED,     ///< 已连接
    DISCONNECTED,  ///< 已断开，等待重连
    RECONNECTING   ///< 重连进行中
};

/**
 * @class ReconnectController
 * @brief 单设备重连控制器
 *
 * 使用指数退避算法计算下一次重连延迟：
 *   delay = min(base * 2^attempt, maxDelay)
 *   base     = Constants::RECONNECT_BASE_DELAY    (1000ms)
 *   maxDelay = Constants::RECONNECT_MAX_DELAY     (32000ms)
 *
 * 连接成功后重置退避状态。
 */
class ReconnectController : public QObject
{
    Q_OBJECT

public:
    explicit ReconnectController(QObject *parent = nullptr);
    ~ReconnectController() override;

    /**
     * @brief 启动重连控制器
     * @param comm   通信实例（裸指针，不拥有所有权）
     * @param config 设备配置
     */
    void start(ICommunication *comm, const DeviceConfig &config);

    /**
     * @brief 停止重连控制器（保留当前状态，可再次 start）
     */
    void stop();

    /**
     * @brief 连接成功回调（由 DeviceManager 调用）
     */
    void onConnected();

    /**
     * @brief 连接断开回调（由 DeviceManager 调用）
     */
    void onDisconnected();

    /**
     * @brief 获取当前状态
     */
    ReconnectState state() const;

    /**
     * @brief 获取当前退避延迟 (ms)
     */
    int currentDelay() const;

signals:
    /**
     * @brief 发起重连尝试
     * @param attempt 当前尝试次数（从 1 开始）
     * @param delay   本次延迟 (ms)
     */
    void reconnectAttempt(int attempt, int delay);

    /**
     * @brief 重连成功
     */
    void reconnected();

    /**
     * @brief 重连失败（达到最大次数）
     */
    void reconnectFailed();

private slots:
    /**
     * @brief 重连定时器超时槽
     */
    void onReconnectTimer();

private:
    /**
     * @brief 根据当前 attempt 计算下一次退避延迟
     * @param attempt 当前尝试次数
     * @return 延迟毫秒数
     */
    int computeDelay(int attempt) const;

    /**
     * @brief 重置退避状态
     */
    void resetBackoff();

    /**
     * @brief 安排下一次重连
     */
    void scheduleReconnect();

    /**
     * @brief 切换内部状态并发射信号
     */
    void transitionTo(ReconnectState newState);

    QString        m_deviceId;      ///< 设备 ID
    QPointer<ICommunication> m_communication;///< 通信实例（非所有权引用，QPointer 守护防止悬空）
    DeviceConfig   m_config;        ///< 设备配置
    QTimer         m_reconnectTimer;///< 重连定时器
    int            m_currentDelay; ///< 当前退避延迟 (ms)
    int            m_attemptCount; ///< 当前尝试次数
    ReconnectState m_state;        ///< 当前状态
};

#endif // DEBUGPLATFORM_RECONNECTCONTROLLER_H
