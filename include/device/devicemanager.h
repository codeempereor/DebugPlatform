#ifndef DEBUGPLATFORM_DEVICEMANAGER_H
#define DEBUGPLATFORM_DEVICEMANAGER_H

/**
 * @file devicemanager.h
 * @brief 设备管理器单例
 *
 * 统一管理多个设备的配置、通信实例和连接状态。
 * 通过 ICommunication 抽象接口转发底层通信信号至设备级信号，
 * 并支持轮询调度、批量连接/断开等高级操作。
 */

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QStringList>
#include "core/config.h"
#include "device/deviceconfig.h"

class ICommunication;
class QThread;
class ReconnectController;

/**
 * @class DeviceManager
 * @brief 设备管理器（单例）
 *
 * 负责：
 *  - 维护设备配置表、通信实例表、设备状态表
 *  - 连接/断开单个或全部设备
 *  - 将 ICommunication 的底层信号转换为设备级信号
 *  - 提供轮询定时器，用于周期触发设备轮询
 */
class DeviceManager : public QObject
{
    Q_OBJECT

    DeviceManager();
    ~DeviceManager() override = default;

public:
    /**
     * @brief 获取单例实例
     * @return DeviceManager 实例引用
     */
    static DeviceManager &instance();

    /**
     * @brief 添加设备（注册配置但不自动连接）
     * @param config 设备配置
     * @return true 添加成功，false 设备已存在或配置非法
     */
    bool addDevice(const DeviceConfig &config);

    /**
     * @brief 移除设备（同时断开并释放通信实例）
     * @param deviceId 设备唯一标识
     * @return true 移除成功
     */
    bool removeDevice(const QString &deviceId);

    /**
     * @brief 连接指定设备
     * @param deviceId 设备唯一标识
     * @return true 连接请求成功发起
     */
    bool connectDevice(const QString &deviceId);

    /**
     * @brief 断开指定设备
     * @param deviceId 设备唯一标识
     * @return true 断开成功
     */
    bool disconnectDevice(const QString &deviceId);

    /**
     * @brief 连接所有已启用的设备
     * @return 成功连接的设备数量
     */
    int connectAll();

    /**
     * @brief 断开所有设备
     */
    void disconnectAll();

    /**
     * @brief 获取指定设备的状态
     * @param deviceId 设备唯一标识
     * @return 当前设备状态
     */
    DeviceStatus getDeviceStatus(const QString &deviceId) const;

    /**
     * @brief 获取当前已连接设备的 ID 列表
     * @return 已连接设备 ID 列表
     */
    QStringList getConnectedDevices() const;

    /**
     * @brief 获取所有已注册设备的 ID 列表（无论连接状态）
     * @return 所有设备 ID 列表
     */
    QStringList getAllDeviceIds() const;

    /**
     * @brief 向指定设备发送命令
     * @param deviceId 设备唯一标识
     * @param command 命令字节
     * @return true 发送成功
     */
    bool sendCommand(const QString &deviceId, const QByteArray &command);

    /**
     * @brief 向所有已连接设备广播命令
     * @param command 命令字节
     * @return 成功发送的设备数量
     */
    int sendCommandToAll(const QByteArray &command);

    /**
     * @brief 获取指定设备的配置
     * @param deviceId 设备唯一标识
     * @return 设备配置（若不存在返回默认构造的配置）
     */
    DeviceConfig getDeviceConfig(const QString &deviceId) const;

    /**
     * @brief 更新设备配置（若设备已连接则会先断开再按新配置重连）
     * @param config 新的设备配置
     */
    void updateDeviceConfig(const DeviceConfig &config);

signals:
    /**
     * @brief 新增/注册一个设备成功后发射
     */
    void deviceAdded(const QString &deviceId);

    /**
     * @brief 设备配置被 updateDeviceConfig 更新后发射
     */
    void deviceConfigChanged(const QString &deviceId);

    /**
     * @brief 设备被从管理器中移除后发射
     */
    void deviceRemoved(const QString &deviceId);

    /**
     * @brief 设备连接成功
     */
    void deviceConnected(const QString &deviceId);

    /**
     * @brief 设备断开
     */
    void deviceDisconnected(const QString &deviceId);

    /**
     * @brief 设备接收到数据
     */
    void deviceDataReceived(const QString &deviceId, const QByteArray &data);

    /**
     * @brief MQTT 类型设备收到发布消息（携带 topic + payload 供 JSON 解析）
     */
    void deviceMqttMessage(const QString &deviceId,
                           const QString &topic,
                           const QByteArray &payload);

    /**
     * @brief 设备发生错误
     */
    void deviceError(const QString &deviceId, const QString &error);

    /**
     * @brief 设备状态变更
     */
    void deviceStatusChanged(const QString &deviceId, DeviceStatus status);

    /**
     * @brief 所有设备均已连接
     */
    void allDevicesConnected();

    /**
     * @brief 所有设备均已断开
     */
    void allDevicesDisconnected();

private slots:
    /**
     * @brief 轮询定时器超时槽
     *        周期性触发，用于驱动设备轮询
     */
    void onPollTimeout();

    // ICommunication 底层信号转设备级信号的转发槽
    void onCommunicationConnected();
    void onCommunicationDisconnected();
    void onCommunicationDataReceived(const QByteArray &data);
    void onCommunicationErrorOccurred(const QString &errorMessage, int errorCode);
    void onCommunicationMqttMessage(const QString &topic, const QByteArray &payload);

private:
    /**
     * @brief 根据通信配置创建 ICommunication 实例
     * @param config 设备配置
     * @return 新建的通信实例，失败返回 nullptr
     */
    ICommunication *createCommunication(const DeviceConfig &config);

    /**
     * @brief 为指定设备建立信号槽转发连接
     * @param deviceId 设备 ID
     */
    void connectCommunicationSignals(const QString &deviceId);

    /**
     * @brief 更新设备状态并发射状态变更信号
     * @param deviceId 设备 ID
     * @param status 新状态
     */
    void setDeviceStatus(const QString &deviceId, DeviceStatus status);

    /**
     * @brief 检查并发射 allDevicesConnected / allDevicesDisconnected
     */
    void checkAllDevicesStatus();

    /**
     * @brief 启动轮询定时器（若尚未启动）
     */
    void startPollTimerIfNeeded();

    /**
     * @brief 停止轮询定时器（无设备连接时）
     */
    void stopPollTimerIfNeeded();

    /**
     * @brief 启动设备专属 IO 线程并把通信实例 moveToThread 到该线程
     * @param deviceId 设备 ID
     * @param comm     通信实例（已创建，未打开）
     * @return true 启动成功
     *
     * P1-1 多线程改造：每设备一个 QThread，ICommunication 实例
     * moveToThread 到该线程，所有 socket 信号槽与 IO 操作均在工作线程执行。
     */
    bool startIoThread(const QString &deviceId, ICommunication *comm);

    /**
     * @brief 停止并清理设备 IO 线程（quit + wait + delete）
     * @param deviceId 设备 ID
     *
     * 调用前必须保证 ICommunication 已经 close()，否则线程可能阻塞。
     */
    void stopIoThread(const QString &deviceId);

    QHash<QString, DeviceConfig>         m_devices;       ///< 设备配置表
    QHash<QString, ICommunication*>      m_communications;///< 通信实例（拥有所有权）
    QHash<QString, DeviceStatus>         m_statuses;      ///< 设备状态
    QHash<QString, ReconnectController*> m_reconnecters;  ///< 重连控制器（拥有所有权）
    QHash<QString, QThread*>         m_ioThreads;     ///< 每设备专属 IO 线程（拥有所有权）
    QTimer m_pollTimer;                                    ///< 轮询定时器

    DISABLE_COPY(DeviceManager)
};

#endif // DEBUGPLATFORM_DEVICEMANAGER_H
