#include "device/reconnectcontroller.h"
#include "core/icommunication.h"

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <algorithm>

// ============================================================================
// 构造/析构
// ============================================================================
ReconnectController::ReconnectController(QObject *parent)
    : QObject(parent)
    , m_deviceId()
    , m_communication(nullptr)
    , m_config()
    , m_currentDelay(0)
    , m_attemptCount(0)
    , m_state(ReconnectState::IDLE)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &ReconnectController::onReconnectTimer);
}

ReconnectController::~ReconnectController()
{
    stop();
}

// ============================================================================
// 公共接口
// ============================================================================
void ReconnectController::start(ICommunication *comm, const DeviceConfig &config)
{
    m_communication = comm;
    m_config        = config;
    m_deviceId      = config.id;

    resetBackoff();
    // 连接已在外部建立（connectDevice 中已 open 成功），
    // 这里直接设为 CONNECTED 状态，等待未来的断线事件
    transitionTo(ReconnectState::CONNECTED);

    if (!m_deviceId.isEmpty()) {
        qDebug() << LogCategory::Device
                 << "ReconnectController: armed for device" << m_deviceId;
    }
}

void ReconnectController::stop()
{
    if (m_reconnectTimer.isActive()) {
        m_reconnectTimer.stop();
    }
    transitionTo(ReconnectState::IDLE);
    resetBackoff();

    m_communication = nullptr;
}

void ReconnectController::onConnected()
{
    if (m_state == ReconnectState::CONNECTED) {
        return;
    }

    // 重置退避
    resetBackoff();
    transitionTo(ReconnectState::CONNECTED);

    if (!m_deviceId.isEmpty()) {
        qDebug() << LogCategory::Device
                 << "ReconnectController: device connected" << m_deviceId;
    }

    emit reconnected();
}

void ReconnectController::onDisconnected()
{
    // IDLE 或已经在重连流程中则忽略
    if (m_state == ReconnectState::IDLE ||
        m_state == ReconnectState::RECONNECTING) {
        return;
    }

    transitionTo(ReconnectState::DISCONNECTED);

    if (m_config.autoReconnect) {
        // 进入重连流程
        scheduleReconnect();
    }
}

ReconnectState ReconnectController::state() const
{
    return m_state;
}

int ReconnectController::currentDelay() const
{
    return m_currentDelay;
}

// ============================================================================
// 内部实现
// ============================================================================
void ReconnectController::onReconnectTimer()
{
    if (!m_communication) {
        qWarning() << LogCategory::Device
                   << "ReconnectController: communication is null";
        stop();
        return;
    }

    // 已连接则忽略重连请求
    if (m_state == ReconnectState::CONNECTED) {
        return;
    }

    // 如果是首次连接，状态为 CONNECTING；否则为 RECONNECTING
    if (m_state == ReconnectState::CONNECTING) {
        // 首次连接尝试
    } else if (m_state == ReconnectState::DISCONNECTED ||
               m_state == ReconnectState::RECONNECTING) {
        transitionTo(ReconnectState::RECONNECTING);
    } else {
        // 其他状态，忽略
        return;
    }

    m_attemptCount++;

    // 尝试打开通信实例：
    //   - 当 comm 在不同线程（生产环境，comm 已 moveToThread 至 IO 线程）：
    //     使用 BlockingQueuedConnection 同步等待结果
    //   - 当 comm 在同一线程（单元测试场景，未启动 IO 线程）：
    //     直接调用，避免 BlockingQueuedConnection 死锁
    bool openOk = false;
    if (m_communication->thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(m_communication, "open",
                                 Qt::BlockingQueuedConnection,
                                 Q_RETURN_ARG(bool, openOk));
    } else {
        openOk = m_communication->open();
    }
    if (openOk) {
        // open 成功，立即切换到 CONNECTED 状态
        onConnected();
        return;
    }

    // 打开失败，安排下一次重连
    if (m_state == ReconnectState::CONNECTING) {
        // 首次连接失败，转入 DISCONNECTED 并尝试重连
        transitionTo(ReconnectState::DISCONNECTED);
    }

    scheduleReconnect();
}

void ReconnectController::scheduleReconnect()
{
    const int delay = computeDelay(m_attemptCount);
    m_currentDelay  = delay;

    emit reconnectAttempt(m_attemptCount, delay);

    qDebug() << LogCategory::Device
             << "ReconnectController: schedule attempt" << m_attemptCount
             << "delay" << delay << "ms"
             << "for device" << m_deviceId;

    // 检查最大尝试次数
    if (m_attemptCount >= Constants::RECONNECT_MAX_ATTEMPTS &&
        m_state != ReconnectState::CONNECTED) {
        qWarning() << LogCategory::Device
                   << "ReconnectController: max attempts reached for device"
                   << m_deviceId;
        emit reconnectFailed();
        transitionTo(ReconnectState::DISCONNECTED);
        return;
    }

    m_reconnectTimer.start(delay);
}

int ReconnectController::computeDelay(int attempt) const
{
    const int base     = Constants::RECONNECT_BASE_DELAY;
    const int maxDelay = Constants::RECONNECT_MAX_DELAY;

    // delay = min(base * 2^(attempt-1), maxDelay)
    // attempt 从 1 开始
    int delay = base;
    for (int i = 1; i < attempt; ++i) {
        delay *= 2;
        if (delay >= maxDelay) {
            return maxDelay;
        }
    }
    return std::min(delay, maxDelay);
}

void ReconnectController::resetBackoff()
{
    m_attemptCount  = 0;
    m_currentDelay  = 0;
    if (m_reconnectTimer.isActive()) {
        m_reconnectTimer.stop();
    }
}

void ReconnectController::transitionTo(ReconnectState newState)
{
    m_state = newState;

    switch (newState) {
    case ReconnectState::IDLE:
        qDebug() << LogCategory::Device << "ReconnectController: state -> IDLE";
        break;
    case ReconnectState::CONNECTING:
        qDebug() << LogCategory::Device << "ReconnectController: state -> CONNECTING";
        break;
    case ReconnectState::CONNECTED:
        qDebug() << LogCategory::Device << "ReconnectController: state -> CONNECTED";
        break;
    case ReconnectState::DISCONNECTED:
        qDebug() << LogCategory::Device << "ReconnectController: state -> DISCONNECTED";
        break;
    case ReconnectState::RECONNECTING:
        qDebug() << LogCategory::Device << "ReconnectController: state -> RECONNECTING";
        break;
    }
}
