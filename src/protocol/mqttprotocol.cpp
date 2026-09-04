#include "protocol/mqttprotocol.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

QList<MqttDataPoint> MqttProtocol::parseMessage(const QString &topic,
                                                  const QByteArray &payload)
{
    QList<MqttDataPoint> result;

    if (payload.isEmpty()) return result;

    // 从 topic 提取默认 deviceId
    QString deviceId = extractDeviceId(topic);
    int topicAddr = extractAddress(topic);

    // 解析 JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

    if (doc.isNull()) {
        // 不是合法 JSON，尝试当作纯数值解析
        bool ok = false;
        double val = QString::fromUtf8(payload).trimmed().toDouble(&ok);
        if (ok && topicAddr >= 0) {
            result.append(MqttDataPoint(deviceId, topicAddr, val));
        }
        return result;
    }

    // 格式1: 数组 {"data": [{"addr": 0, "value": 123.45}, ...]}
    if (doc.isObject()) {
        QJsonObject obj = doc.object();

        if (obj.contains("data") && obj.value("data").isArray()) {
            QJsonArray arr = obj.value("data").toArray();
            for (const QJsonValue &item : arr) {
                if (item.isObject()) {
                    QJsonObject itemObj = item.toObject();
                    int addr = itemObj.value("addr").toInt(topicAddr);
                    double val = itemObj.value("value").toDouble(0.0);
                    QString dev = itemObj.value("deviceId").toString(deviceId);
                    result.append(MqttDataPoint(dev, addr, val));
                }
            }
        }
        // 格式2: 单数据点 {"addr": 0, "value": 123.45}
        else if (obj.contains("value")) {
            int addr = obj.value("addr").toInt(topicAddr);
            double val = obj.value("value").toDouble(0.0);
            QString dev = obj.value("deviceId").toString(deviceId);
            result.append(MqttDataPoint(dev, addr, val));
        }
        // 格式3: {"deviceId": "xxx", "addr": 0, "value": 123.45} (已被格式2覆盖)
    }

    return result;
}

QString MqttProtocol::extractDeviceId(const QString &topic)
{
    // 格式: devices/{deviceId}/data/...
    QStringList parts = topic.split('/');
    if (parts.size() >= 2 && parts[0] == "devices") {
        return parts[1];
    }
    // 也支持 {deviceId}/data/... 格式
    if (parts.size() >= 2 && parts[1] == "data") {
        return parts[0];
    }
    return QString();
}

int MqttProtocol::extractAddress(const QString &topic)
{
    // 格式: .../data/{addr}
    QStringList parts = topic.split('/');
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (parts[i] == "data") {
            bool ok = false;
            int addr = parts[i + 1].toInt(&ok);
            if (ok) {
                return addr;
            }
            // "data" 后跟的不是有效数字，视为未找到
            return -1;
        }
    }
    return -1;
}

QByteArray MqttProtocol::buildJsonPayload(int addr, double value)
{
    QJsonObject obj;
    obj["addr"] = addr;
    obj["value"] = value;
    QJsonDocument doc(obj);
    return doc.toJson(QJsonDocument::Compact);
}

QByteArray MqttProtocol::buildJsonPayload(const QList<QPair<int, double>> &points)
{
    QJsonArray arr;
    for (const auto &p : points) {
        QJsonObject item;
        item["addr"] = p.first;
        item["value"] = p.second;
        arr.append(item);
    }
    QJsonObject root;
    root["data"] = arr;
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}