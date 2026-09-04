#include "device/deviceconfig.h"

#include <QDebug>

// ============================================================================
// toMap - 将 DeviceConfig 序列化为 QVariantMap
// ============================================================================
QVariantMap DeviceConfig::toMap() const
{
    QVariantMap map;

    map["id"]               = id;
    map["name"]             = name;
    map["commType"]         = static_cast<int>(commType);
    map["protocolType"]     = static_cast<int>(protocolType);
    map["pollInterval"]     = pollInterval;
    map["timeout"]          = timeout;
    map["autoReconnect"]    = autoReconnect;
    map["enabled"]          = enabled;
    map["customProperties"] = customProperties;

    // 序列化通信配置
    QVariantMap commMap;
    commMap["type"] = static_cast<int>(commConfig.type);

    QVariantMap serialMap;
    serialMap["portName"]      = commConfig.serial.portName;
    serialMap["baudRate"]      = commConfig.serial.baudRate;
    serialMap["dataBits"]      = static_cast<int>(commConfig.serial.dataBits);
    serialMap["stopBits"]      = static_cast<int>(commConfig.serial.stopBits);
    serialMap["parity"]        = static_cast<int>(commConfig.serial.parity);
    serialMap["flowControl"]   = static_cast<int>(commConfig.serial.flowControl);
    commMap["serial"] = serialMap;

    QVariantMap tcpMap;
    tcpMap["hostAddress"]       = commConfig.tcp.hostAddress;
    tcpMap["port"]              = commConfig.tcp.port;
    tcpMap["connectionTimeout"] = commConfig.tcp.connectionTimeout;
    commMap["tcp"] = tcpMap;

    QVariantMap udpMap;
    udpMap["hostAddress"] = commConfig.udp.hostAddress;
    udpMap["port"]        = commConfig.udp.port;
    udpMap["localPort"]   = commConfig.udp.localPort;
    commMap["udp"] = udpMap;

    QVariantMap mqttMap;
    mqttMap["brokerHost"]      = commConfig.mqtt.brokerHost;
    mqttMap["brokerPort"]      = commConfig.mqtt.brokerPort;
    mqttMap["clientId"]        = commConfig.mqtt.clientId;
    mqttMap["username"]        = commConfig.mqtt.username;
    mqttMap["password"]        = commConfig.mqtt.password;
    mqttMap["subscribeTopic"]  = commConfig.mqtt.subscribeTopic;
    mqttMap["publishTopic"]    = commConfig.mqtt.publishTopic;
    mqttMap["qos"]             = commConfig.mqtt.qos;
    mqttMap["keepAlive"]       = commConfig.mqtt.keepAlive;
    mqttMap["cleanSession"]    = commConfig.mqtt.cleanSession;
    commMap["mqtt"] = mqttMap;

    map["commConfig"] = commMap;

    return map;
}

// ============================================================================
// fromMap - 从 QVariantMap 反序列化 DeviceConfig
// ============================================================================
void DeviceConfig::fromMap(const QVariantMap &map)
{
    id            = map.value("id").toString();
    name          = map.value("name").toString();
    commType      = static_cast<CommunicationType>(map.value("commType").toInt());
    protocolType  = static_cast<ProtocolType>(map.value("protocolType").toInt());
    pollInterval  = map.value("pollInterval", 100).toInt();
    timeout       = map.value("timeout", 1000).toInt();
    autoReconnect = map.value("autoReconnect", false).toBool();
    enabled       = map.value("enabled", true).toBool();
    customProperties = map.value("customProperties").toMap();

    // 反序列化通信配置
    const QVariantMap commMap = map.value("commConfig").toMap();
    if (!commMap.isEmpty()) {
        commConfig.type = static_cast<CommunicationType>(commMap.value("type").toInt());

        const QVariantMap serialMap = commMap.value("serial").toMap();
        if (!serialMap.isEmpty()) {
            commConfig.serial.portName    = serialMap.value("portName").toString();
            commConfig.serial.baudRate    = serialMap.value("baudRate").toInt();
            commConfig.serial.dataBits    = static_cast<QSerialPort::DataBits>(serialMap.value("dataBits").toInt());
            commConfig.serial.stopBits    = static_cast<QSerialPort::StopBits>(serialMap.value("stopBits").toInt());
            commConfig.serial.parity      = static_cast<QSerialPort::Parity>(serialMap.value("parity").toInt());
            commConfig.serial.flowControl = static_cast<QSerialPort::FlowControl>(serialMap.value("flowControl").toInt());
        }

        const QVariantMap tcpMap = commMap.value("tcp").toMap();
        if (!tcpMap.isEmpty()) {
            commConfig.tcp.hostAddress       = tcpMap.value("hostAddress").toString();
            commConfig.tcp.port              = tcpMap.value("port").toInt();
            commConfig.tcp.connectionTimeout = tcpMap.value("connectionTimeout").toInt();
        }

        const QVariantMap udpMap = commMap.value("udp").toMap();
        if (!udpMap.isEmpty()) {
            commConfig.udp.hostAddress = udpMap.value("hostAddress").toString();
            commConfig.udp.port        = udpMap.value("port").toInt();
            commConfig.udp.localPort   = udpMap.value("localPort").toInt();
        }

        const QVariantMap mqttMap = commMap.value("mqtt").toMap();
        if (!mqttMap.isEmpty()) {
            commConfig.mqtt.brokerHost     = mqttMap.value("brokerHost").toString();
            commConfig.mqtt.brokerPort     = mqttMap.value("brokerPort").toInt();
            commConfig.mqtt.clientId       = mqttMap.value("clientId").toString();
            commConfig.mqtt.username       = mqttMap.value("username").toString();
            commConfig.mqtt.password       = mqttMap.value("password").toString();
            commConfig.mqtt.subscribeTopic = mqttMap.value("subscribeTopic").toString();
            commConfig.mqtt.publishTopic   = mqttMap.value("publishTopic").toString();
            commConfig.mqtt.qos            = mqttMap.value("qos").toInt();
            commConfig.mqtt.keepAlive      = mqttMap.value("keepAlive").toInt();
            commConfig.mqtt.cleanSession   = mqttMap.value("cleanSession").toBool();
        }
    }
}

// ============================================================================
// toString - 可读字符串表示
// ============================================================================
QString DeviceConfig::toString() const
{
    QStringList parts;
    parts << QString("id=%1").arg(id);
    parts << QString("name=%1").arg(name);
    parts << QString("comm=%1").arg(commConfig.toString());

    switch (protocolType) {
    case ProtocolType::BinaryFrame: parts << "protocol=BinaryFrame"; break;
    case ProtocolType::ModbusRtu:    parts << "protocol=ModbusRtu";    break;
    case ProtocolType::ModbusTcp:    parts << "protocol=ModbusTcp";    break;
    case ProtocolType::MqttJson:     parts << "protocol=MqttJson";     break;
    }

    parts << QString("poll=%1ms").arg(pollInterval);
    parts << QString("timeout=%1ms").arg(timeout);
    parts << QString("autoReconnect=%1").arg(autoReconnect ? "yes" : "no");
    parts << QString("enabled=%1").arg(enabled ? "yes" : "no");

    return QString("DeviceConfig{%1}").arg(parts.join(", "));
}
