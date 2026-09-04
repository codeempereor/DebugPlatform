#ifndef DEBUGPLATFORM_COMMUNICATION_CONFIG_H
#define DEBUGPLATFORM_COMMUNICATION_CONFIG_H

/**
 * @file communication_config.h
 * @brief 通信配置结构定义
 *
 * 定义串口、TCP、UDP 等通信方式的配置参数。
 */

#include <QString>
#include <QHostAddress>
#include <QSerialPort>
#include "config.h"

// ============================================================================
// 串口配置
// ============================================================================
struct SerialConfig {
    QString portName = "COM1";          // 串口号
    qint32 baudRate = 115200;            // 波特率
    QSerialPort::DataBits dataBits = QSerialPort::Data8;  // 数据位
    QSerialPort::StopBits stopBits = QSerialPort::OneStop; // 停止位
    QSerialPort::Parity parity = QSerialPort::NoParity;    // 校验位
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl; // 流控
    
    bool operator==(const SerialConfig& other) const {
        return portName == other.portName
            && baudRate == other.baudRate
            && dataBits == other.dataBits
            && stopBits == other.stopBits
            && parity == other.parity
            && flowControl == other.flowControl;
    }
};

// ============================================================================
// TCP 配置
// ============================================================================
struct TcpConfig {
    QString hostAddress = "127.0.0.1";  // 主机地址
    quint16 port = 502;                  // 端口号
    int connectionTimeout = 3000;        // 连接超时 (ms)
    
    bool operator==(const TcpConfig& other) const {
        return hostAddress == other.hostAddress
            && port == other.port
            && connectionTimeout == other.connectionTimeout;
    }
};

// ============================================================================
// UDP 配置
// ============================================================================
struct UdpConfig {
    QString hostAddress = "127.0.0.1";  // 目标地址
    quint16 port = 502;                  // 目标端口
    quint16 localPort = 0;               // 本地端口 (0 = 自动分配)
    
    bool operator==(const UdpConfig& other) const {
        return hostAddress == other.hostAddress
            && port == other.port
            && localPort == other.localPort;
    }
};

// ============================================================================
// MQTT 配置
// ============================================================================
struct MqttConfig {
    QString brokerHost = "broker.emqx.io";  // Broker 地址
    quint16 brokerPort = 1883;               // Broker 端口
    QString clientId = "DebugPlatform";      // 客户端ID
    QString username;                         // 用户名（可选）
    QString password;                         // 密码（可选）
    QString subscribeTopic = "devices/+/data"; // 订阅主题
    QString publishTopic;                     // 发布主题
    int qos = 0;                             // QoS 级别 (0/1/2)
    int keepAlive = 60;                      // 心跳间隔（秒）
    bool cleanSession = true;                // 清除会话

    bool operator==(const MqttConfig& other) const {
        return brokerHost == other.brokerHost
            && brokerPort == other.brokerPort
            && clientId == other.clientId
            && username == other.username
            && password == other.password
            && subscribeTopic == other.subscribeTopic
            && publishTopic == other.publishTopic
            && qos == other.qos
            && keepAlive == other.keepAlive
            && cleanSession == other.cleanSession;
    }
};
// ============================================================================
// 统一通信配置
// ============================================================================
struct CommunicationConfig {
    CommunicationType type = CommunicationType::Serial;
    
    SerialConfig serial;
    TcpConfig tcp;
    UdpConfig udp;
    MqttConfig mqtt;
    
    QString toString() const {
        switch (type) {
        case CommunicationType::Serial:
            return QString("Serial(%1@%2)").arg(serial.portName).arg(serial.baudRate);
        case CommunicationType::Tcp:
            return QString("TCP(%1:%2)").arg(tcp.hostAddress).arg(tcp.port);
        case CommunicationType::Udp:
            return QString("UDP(%1:%2)").arg(udp.hostAddress).arg(udp.port);
        case CommunicationType::Mqtt:
            return QString("MQTT(%1:%2, topic=%3)").arg(mqtt.brokerHost).arg(mqtt.brokerPort).arg(mqtt.subscribeTopic);
        }
        return "Unknown";
    }
};

#endif // DEBUGPLATFORM_COMMUNICATION_CONFIG_H
