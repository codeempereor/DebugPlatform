#ifndef MQTTPROTOCOL_H
#define MQTTPROTOCOL_H

#include <QByteArray>
#include <QString>
#include <QList>
#include <QPair>

/**
 * @brief MQTT 数据点结构
 */
struct MqttDataPoint {
    QString deviceId;   // 设备ID (从 topic 或 JSON 提取)
    int address;        // 寄存器地址
    double value;       // 寄存器值

    MqttDataPoint() : address(0), value(0.0) {}
    MqttDataPoint(const QString &dev, int addr, double val)
        : deviceId(dev), address(addr), value(val) {}
};

/**
 * @brief MQTT JSON 协议解析器
 *
 * 解析 MQTT PUBLISH 消息中的 JSON payload，提取数据点。
 * 支持三种 JSON 格式：
 * 1. 单数据点: {"addr": 0, "value": 123.45}
 * 2. 数组: {"data": [{"addr": 0, "value": 123.45}, ...]}
 * 3. 简单值 (addr 从 topic 提取): {"value": 123.45}
 *
 * Topic 格式约定: devices/{deviceId}/data/{addr}
 */
class MqttProtocol
{
public:
    MqttProtocol() = delete;  // 静态工具类

    /**
     * @brief 解析 JSON payload 提取数据点列表
     * @param topic MQTT 主题 (用于提取 deviceId 和 addr)
     * @param payload JSON 格式的消息内容
     * @return 解析出的数据点列表 (空列表表示解析失败或无数据)
     */
    static QList<MqttDataPoint> parseMessage(const QString &topic,
                                              const QByteArray &payload);

    /**
     * @brief 从 topic 中提取 deviceId
     * @param topic MQTT 主题 (格式: devices/{deviceId}/data/...)
     * @return deviceId 字符串 (找不到返回空)
     */
    static QString extractDeviceId(const QString &topic);

    /**
     * @brief 从 topic 中提取寄存器地址
     * @param topic MQTT 主题 (格式: .../data/{addr})
     * @return 地址 (找不到返回 -1)
     */
    static int extractAddress(const QString &topic);

    /**
     * @brief 构建 JSON payload (单数据点)
     * @param addr 寄存器地址
     * @param value 寄存器值
     * @return JSON 格式的 QByteArray
     */
    static QByteArray buildJsonPayload(int addr, double value);

    /**
     * @brief 构建 JSON payload (多数据点)
     * @param points 数据点列表
     * @return JSON 格式的 QByteArray
     */
    static QByteArray buildJsonPayload(const QList<QPair<int, double>> &points);
};

#endif // MQTTPROTOCOL_H