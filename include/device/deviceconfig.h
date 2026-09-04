#ifndef DEBUGPLATFORM_DEVICECONFIG_H
#define DEBUGPLATFORM_DEVICECONFIG_H

/**
 * @file deviceconfig.h
 * @brief 设备配置结构定义
 *
 * 定义单个设备的完整配置信息，包括通信参数、协议类型、
 * 轮询策略等。支持序列化/反序列化，便于配置的持久化存储。
 */

#include <QString>
#include <QVariantMap>
#include "core/config.h"
#include "core/communication_config.h"

/**
 * @struct DeviceConfig
 * @brief 设备配置结构
 *
 * 包含一个设备所需的全部配置信息：唯一标识、通信方式、
 * 协议类型、轮询策略、自动重连选项以及可扩展的自定义属性。
 */
struct DeviceConfig
{
    QString id;                  ///< 设备唯一标识
    QString name;                ///< 设备名称
    CommunicationType commType = CommunicationType::Serial;  ///< 通信类型
    CommunicationConfig commConfig;   ///< 通信配置
    ProtocolType protocolType = ProtocolType::BinaryFrame;  ///< 协议类型
    int pollInterval = 100;      ///< 轮询间隔(ms)，默认100
    int timeout = 1000;          ///< 超时(ms)，默认1000
    bool autoReconnect = false;  ///< 是否自动重连
    bool enabled = true;         ///< 是否启用
    QVariantMap customProperties;///< 自定义属性

    /**
     * @brief 将配置转换为 QVariantMap
     * @return 可序列化的配置映射
     */
    QVariantMap toMap() const;

    /**
     * @brief 从 QVariantMap 加载配置
     * @param map 配置映射
     */
    void fromMap(const QVariantMap &map);

    /**
     * @brief 获取配置的可读字符串表示
     * @return 描述设备配置的字符串
     */
    QString toString() const;
};

#endif // DEBUGPLATFORM_DEVICECONFIG_H
