#include "device/devicemanager.h"
#include "device/reconnectcontroller.h"
#include "core/communicationfactory.h"
#include "core/mqttcommunication.h"
#include "core/icommunication.h"
#include "data/logmanager.h"
#include "core/config.h"

#include <QDebug>
#include <QThread>
#include <QMetaObject>

// ============================================================================
// 构造/析构
// ============================================================================
DeviceManager &DeviceManager::instance()
{
    static DeviceManager s_instance;
    return s_instance;
}

DeviceManager::DeviceManager()
    : QObject(nullptr)
{
    m_pollTimer.setInterval(Constants::DEFAULT_POLL_INTERVAL);
    m_pollTimer.setSingleShot(false);
    connect(&m_pollTimer, &QTimer::timeout,
            this, &DeviceManager::onPollTimeout);
}

// ============================================================================
// 设备管理 API
// ============================================================================
bool DeviceManager::addDevice(const DeviceConfig &config)
{
    if (config.id.isEmpty()) {
        qWarning() << LogCategory::Device << "addDevice: device id is empty";
        return false;
    }

    if (m_devices.contains(config.id)) {
        qWarning() << LogCategory::Device << "addDevice: device already exists:"
                   << config.id;
        return false;
    }

    m_devices.insert(config.id, config);
    m_statuses.insert(config.id, DeviceStatus::Offline);

    qDebug() << LogCategory::Device << "Added device:" << config.toString();
    emit deviceAdded(config.id);
    return true;
}

bool DeviceManager::removeDevice(const QString &deviceId)
{
    if (!m_devices.contains(deviceId)) {
        qWarning() << LogCategory::Device << "removeDevice: device not found:"
                   << deviceId;
        return false;
    }

    // 先断开
    disconnectDevice(deviceId);

    // 释放重连控制器
    if (m_reconnecters.contains(deviceId)) {
        ReconnectController *rc = m_reconnecters.take(deviceId);
        rc->stop();
        delete rc;
    }

    m_statuses.remove(deviceId);
    m_devices.remove(deviceId);

    qDebug() << LogCategory::Device << "Removed device:" << deviceId;
    emit deviceRemoved(deviceId);
    return true;
}

bool DeviceManager::connectDevice(const QString &deviceId)
{
    if (!m_devices.contains(deviceId)) {
        qWarning() << LogCategory::Device << "connectDevice: device not found:"
                   << deviceId;
        return false;
    }

    const DeviceConfig &config = m_devices[deviceId];
    if (!config.enabled) {
        qWarning() << LogCategory::Device << "connectDevice: device is disabled:"
                   << deviceId;
        return false;
    }

    // 如果已有通信实例，先关闭并释放
    if (m_communications.contains(deviceId)) {
        ICommunication *old = m_communications.take(deviceId);
        old->close();
        delete old;
    }

    setDeviceStatus(deviceId, DeviceStatus::Connecting);

    // 创建通信实例
    ICommunication *comm = createCommunication(config);
    if (!comm) {
        setDeviceStatus(deviceId, DeviceStatus::Error);
        return false;
    }

    // P1-1 多线程：启动专属 IO 线程，moveToThread 通信实例
    // 注意：comm 不能 setParent(this)（DeviceManager 在主线程，跨线程 parent 非法）
    m_communications.insert(deviceId, comm);

    // 连接底层信号到设备级信号（AutoConnection：跨线程自动转 Queued）
    connectCommunicationSignals(deviceId);

    // 启动 IO 线程并把 comm 移到该线程
    if (!startIoThread(deviceId, comm)) {
        setDeviceStatus(deviceId, DeviceStatus::Error);
        return false;
    }

    // 跨线程调用 open()：BlockingQueuedConnection 保留同步语义与 bool 返回值
    // open() 内部会懒创建 QSerialPort/QTcpSocket/QUdpSocket（在工作线程中创建）
    bool openOk = false;
    QMetaObject::invokeMethod(comm, "open",
                             Qt::BlockingQueuedConnection,
                             Q_RETURN_ARG(bool, openOk));
    if (!openOk) {
        const DeviceConfig cfg = getDeviceConfig(deviceId);
        const QString tip = cfg.id.isEmpty()
            ? deviceId
            : QStringLiteral("%1（%2）").arg(cfg.name, deviceId);
        LogManager::instance().log(LogLevel::Warning, LogCategory::Device,
            QString("设备连接失败：%1 — 请检查配置或确认对端在线。"
                    "（详细错误已在上方 [通信错误] 日志中给出）").arg(tip));
        setDeviceStatus(deviceId, DeviceStatus::Error);
        // 清理已启动的 IO 线程
        stopIoThread(deviceId);
        m_communications.remove(deviceId);
        return false;
    }


    // open() 成功：同步设置 Online 状态，不依赖异步 connected() 信号
    // （connected() 信号在 BlockingQueuedConnection 模式下可能被延迟处理，
    //   导致 errorOccurred 信号先到达并覆盖状态为 Error）
    setDeviceStatus(deviceId, DeviceStatus::Online);
    // 启动自动重连控制器
    if (config.autoReconnect) {
        ReconnectController *rc = new ReconnectController(this);
        m_reconnecters.insert(deviceId, rc);
        rc->start(comm, config);
    }

    startPollTimerIfNeeded();

    qDebug() << LogCategory::Device << "Connecting device:" << deviceId;
    return true;
}

bool DeviceManager::disconnectDevice(const QString &deviceId)
{
    if (!m_devices.contains(deviceId)) {
        return false;
    }

    // 停止重连控制器（deleteLater 避免与定时器回调竞争）
    if (m_reconnecters.contains(deviceId)) {
        ReconnectController *rc = m_reconnecters.take(deviceId);
        if (rc) {
            // 确保在主线程安全停止定时器
            if (rc->thread() && rc->thread() != QThread::currentThread()) {
                QMetaObject::invokeMethod(rc, "stop", Qt::BlockingQueuedConnection);
            } else {
                rc->stop();
            }
            rc->deleteLater();
        }
    }

    if (m_communications.contains(deviceId)) {
        ICommunication *comm = m_communications.value(deviceId);
        // 跨线程 close()：BlockingQueuedConnection 保证 close 完成后再 quit 线程
        QMetaObject::invokeMethod(comm, "close", Qt::BlockingQueuedConnection);
        // stopIoThread 内部会 take + delete comm，避免重复释放
        stopIoThread(deviceId);
    }

    setDeviceStatus(deviceId, DeviceStatus::Offline);

    stopPollTimerIfNeeded();

    qDebug() << LogCategory::Device << "Disconnected device:" << deviceId;
    return true;
}

int DeviceManager::connectAll()
{
    int connectedCount = 0;
    const QStringList ids = m_devices.keys();
    for (const QString &id : ids) {
        if (m_devices[id].enabled) {
            if (connectDevice(id)) {
                connectedCount++;
            }
        }
    }
    return connectedCount;
}

void DeviceManager::disconnectAll()
{
    const QStringList ids = m_communications.keys();
    for (const QString &id : ids) {
        disconnectDevice(id);
    }
}

DeviceStatus DeviceManager::getDeviceStatus(const QString &deviceId) const
{
    return m_statuses.value(deviceId, DeviceStatus::Offline);
}

QStringList DeviceManager::getConnectedDevices() const
{
    QStringList result;
    for (auto it = m_statuses.cbegin(); it != m_statuses.cend(); ++it) {
        if (it.value() == DeviceStatus::Online) {
            result.append(it.key());
        }
    }
    return result;
}

QStringList DeviceManager::getAllDeviceIds() const
{
    return m_devices.keys();
}

bool DeviceManager::sendCommand(const QString &deviceId,
                                const QByteArray &command)
{
    if (!m_communications.contains(deviceId)) {
        qWarning() << LogCategory::Device << "sendCommand: device not connected:"
                   << deviceId;
        return false;
    }

    ICommunication *comm = m_communications.value(deviceId);
    if (!comm) {
        return false;
    }

    // 跨线程 sendData：QueuedConnection 异步派发，不阻塞 UI
    // 同步检查 isOpen 时，用 BlockingQueuedConnection 拿到当前状态
    bool opened = false;
    QMetaObject::invokeMethod(comm, "isOpen",
                             Qt::BlockingQueuedConnection,
                             Q_RETURN_ARG(bool, opened));
    if (!opened) {
        return false;
    }

    // 派发到工作线程异步发送（fire-and-forget，sendData 内部失败会发 errorOccurred 信号）
    QMetaObject::invokeMethod(comm, "sendData",
                             Qt::QueuedConnection,
                             Q_ARG(QByteArray, command));
    return true;
}

int DeviceManager::sendCommandToAll(const QByteArray &command)
{
    int successCount = 0;
    const QStringList connected = getConnectedDevices();
    for (const QString &id : connected) {
        if (sendCommand(id, command)) {
            successCount++;
        }
    }
    return successCount;
}

DeviceConfig DeviceManager::getDeviceConfig(const QString &deviceId) const
{
    return m_devices.value(deviceId, DeviceConfig());
}

void DeviceManager::updateDeviceConfig(const DeviceConfig &config)
{
    if (!m_devices.contains(config.id)) {
        qWarning() << LogCategory::Device
                   << "updateDeviceConfig: device not found:" << config.id;
        return;
    }

    const bool wasConnected = m_communications.contains(config.id);
    if (wasConnected) {
        disconnectDevice(config.id);
    }

    m_devices.insert(config.id, config);
    emit deviceConfigChanged(config.id);

    if (wasConnected && config.enabled) {
        connectDevice(config.id);
    }
}

// ============================================================================
// 内部实现
// ============================================================================
ICommunication *DeviceManager::createCommunication(const DeviceConfig &config)
{
    return CommunicationFactory::create(config.commType, config.commConfig);
}

void DeviceManager::connectCommunicationSignals(const QString &deviceId)
{
    if (!m_communications.contains(deviceId)) return;

    ICommunication *comm = m_communications.value(deviceId);

    connect(comm, &ICommunication::connected,
            this, &DeviceManager::onCommunicationConnected);
    connect(comm, &ICommunication::disconnected,
            this, &DeviceManager::onCommunicationDisconnected);
    connect(comm, &ICommunication::dataReceived,
            this, &DeviceManager::onCommunicationDataReceived);
    connect(comm, &ICommunication::errorOccurred,
            this, &DeviceManager::onCommunicationErrorOccurred);

    // MQTT 专用：如果底层是 MqttCommunication，连 messageReceived(topic, payload)
    if (auto *mqttComm = qobject_cast<MqttCommunication *>(comm)) {
        connect(mqttComm, &MqttCommunication::messageReceived,
                this, &DeviceManager::onCommunicationMqttMessage);
    }
}

void DeviceManager::setDeviceStatus(const QString &deviceId,
                                    DeviceStatus status)
{
    if (m_statuses.value(deviceId) == status) {
        return;
    }

    m_statuses.insert(deviceId, status);
    emit deviceStatusChanged(deviceId, status);

    switch (status) {
    case DeviceStatus::Online:
        emit deviceConnected(deviceId);
        break;
    case DeviceStatus::Offline:
        emit deviceDisconnected(deviceId);
        break;
    case DeviceStatus::Error:
        emit deviceError(deviceId, "Device entered error state");
        break;
    case DeviceStatus::Connecting:
        break;
    }

    checkAllDevicesStatus();
}

void DeviceManager::checkAllDevicesStatus()
{
    if (m_devices.isEmpty()) return;

    bool anyOnline   = false;
    bool allConnected = true;
    bool anyDevice   = false;

    for (auto it = m_devices.cbegin(); it != m_devices.cend(); ++it) {
        anyDevice = true;
        if (!it.value().enabled) continue;

        const DeviceStatus st = m_statuses.value(it.key(), DeviceStatus::Offline);
        if (st == DeviceStatus::Online) {
            anyOnline = true;
        } else {
            allConnected = false;
        }
    }

    if (!anyDevice) return;

    if (allConnected && anyOnline) {
        emit allDevicesConnected();
    } else if (!anyOnline) {
        emit allDevicesDisconnected();
    }
}

void DeviceManager::startPollTimerIfNeeded()
{
    if (!m_pollTimer.isActive()) {
        m_pollTimer.start();
        qDebug() << LogCategory::Device << "Poll timer started";
    }
}

void DeviceManager::stopPollTimerIfNeeded()
{
    if (m_communications.isEmpty() && m_pollTimer.isActive()) {
        m_pollTimer.stop();
        qDebug() << LogCategory::Device << "Poll timer stopped";
    }
}

// ============================================================================
// P1-1 多线程：IO 线程生命周期
// ============================================================================
bool DeviceManager::startIoThread(const QString &deviceId, ICommunication *comm)
{
    if (!comm) return false;

    // 若已有线程，先清理
    if (m_ioThreads.contains(deviceId)) {
        stopIoThread(deviceId);
    }

    QThread *thread = new QThread();
    // 线程结束时自动清理 comm（comm 的 parent 为 thread，但更稳妥的做法是连接 destroyed）
    comm->moveToThread(thread);

    // 线程结束后自动 delete 线程对象
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();

    // 等待事件循环就绪（避免 BlockingQueuedConnection 在事件循环启动前调用导致死锁）
    // Qt::BlockingQueuedConnection 在事件循环未启动时会阻塞等待，所以这里不强制等待
    // 实际上 invokeMethod 会把事件入队，等事件循环启动后处理

    m_ioThreads.insert(deviceId, thread);

    qDebug() << LogCategory::Device
             << "IO thread started for device" << deviceId
             << "(thread:" << thread << ")";

    // comm 由 DeviceManager 管理（不在 m_communications 中 take 后 delete）
    // 把 comm 的 destroyed 信号连接到线程 cleanup
    connect(comm, &QObject::destroyed, this, [this, deviceId](QObject *) {
        // comm 已被外部 delete，这里只清理 m_communications 残留指针
        // 实际 stopIoThread 会处理线程清理
        Q_UNUSED(deviceId)
    });

    return true;
}

void DeviceManager::stopIoThread(const QString &deviceId)
{
    if (!m_ioThreads.contains(deviceId)) return;

    QThread *thread = m_ioThreads.take(deviceId);
    if (thread) {
        // 通知线程事件循环退出
        thread->quit();
        // 等待线程结束（最多 3 秒）
        if (!thread->wait(3000)) {
            qWarning() << LogCategory::Device
                       << "IO thread did not finish in 3s for device" << deviceId
                       << ", terminating";
            thread->terminate();
            thread->wait(1000);
        }

        // 线程的 deleteLater 会在主线程事件循环中执行
        // thread 对象本身由 finished→deleteLater 自动清理
        // 这里不手动 delete thread，避免 finished 信号中重复 delete

        qDebug() << LogCategory::Device
                 << "IO thread stopped for device" << deviceId;
    }

    // 同步删除 comm（如果还存在）
    // 注意：m_communications 中的指针可能已在外部被 take 走，这里只做安全检查
    if (m_communications.contains(deviceId)) {
        ICommunication *comm = m_communications.take(deviceId);
        if (comm) {
            // comm 已经 moveToThread 到工作线程，现在工作线程已退出
            // 可以安全 delete（此时 comm 处于无线程状态，delete 会触发 destroyed 信号）
            delete comm;
        }
    }
}
// ============================================================================
// 槽实现
// ============================================================================
void DeviceManager::onPollTimeout()
{
    // 轮询定时器目前作为扩展点保留，后续可在此触发设备轮询命令
    // 例如：遍历设备并根据 pollInterval 策略下发轮询帧
}

void DeviceManager::onCommunicationConnected()
{
    ICommunication *comm = qobject_cast<ICommunication*>(sender());
    if (!comm) return;

    const QString deviceId = m_communications.key(comm);
    if (deviceId.isEmpty()) return;

    setDeviceStatus(deviceId, DeviceStatus::Online);

    // 通知重连控制器
    if (m_reconnecters.contains(deviceId)) {
        m_reconnecters[deviceId]->onConnected();
    }
}

void DeviceManager::onCommunicationDisconnected()
{
    ICommunication *comm = qobject_cast<ICommunication*>(sender());
    if (!comm) return;

    const QString deviceId = m_communications.key(comm);
    if (deviceId.isEmpty()) return;

    setDeviceStatus(deviceId, DeviceStatus::Offline);

    // 通知重连控制器（若启用自动重连，由其决定是否重连）
    if (m_reconnecters.contains(deviceId)) {
        m_reconnecters[deviceId]->onDisconnected();
    }
}

void DeviceManager::onCommunicationDataReceived(const QByteArray &data)
{
    ICommunication *comm = qobject_cast<ICommunication*>(sender());
    if (!comm) return;

    const QString deviceId = m_communications.key(comm);
    if (deviceId.isEmpty()) return;

    emit deviceDataReceived(deviceId, data);
}

void DeviceManager::onCommunicationMqttMessage(const QString &topic,
                                                const QByteArray &payload)
{
    ICommunication *comm = qobject_cast<ICommunication*>(sender());
    if (!comm) return;

    const QString deviceId = m_communications.key(comm);
    if (deviceId.isEmpty()) return;

    emit deviceMqttMessage(deviceId, topic, payload);
}

void DeviceManager::onCommunicationErrorOccurred(const QString &errorMessage,
                                                  int errorCode)
{
    ICommunication *comm = qobject_cast<ICommunication*>(sender());
    if (!comm) return;

    const QString deviceId = m_communications.key(comm);
    if (deviceId.isEmpty()) return;

    // 统一写日志（P1 修复：之前这里没打日志，LogWidget 看不到详细错误）
    LogManager::instance().log(LogLevel::Warning, LogCategory::Device,
        QString("[通信错误] 设备%1 (code=%2)：%3")
            .arg(deviceId).arg(errorCode).arg(errorMessage));


    // 状态守卫：如果设备已经是 Online 状态（open() 成功后同步设置的），
    // 则不再将其覆盖为 Error，避免竞态条件下 errorOccurred 信号覆盖正确状态
    // 只有当设备处于 Connecting 或其他非 Online 状态时才更新为 Error
    const DeviceStatus currentStatus = getDeviceStatus(deviceId);
    if (currentStatus == DeviceStatus::Online) {
        LogManager::instance().log(LogLevel::Debug, LogCategory::Device,
            QString("[状态守卫] 设备%1 已在线，忽略延迟到达的错误信号 (code=%2)")
                .arg(deviceId).arg(errorCode));
        return;
    }

    emit deviceError(deviceId, errorMessage);
    setDeviceStatus(deviceId, DeviceStatus::Error);
}
