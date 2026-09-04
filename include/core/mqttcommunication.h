#ifndef DEBUGPLATFORM_MQTTCOMMUNICATION_H
#define DEBUGPLATFORM_MQTTCOMMUNICATION_H

/**
 * @file mqttcommunication.h
 * @brief MQTT 3.1.1 客户端通信实现类
 *
 * 基于 QTcpSocket 手写 MQTT 3.1.1 协议栈，实现 CONNECT/CONNACK、
 * SUBSCRIBE/SUBACK、PUBLISH/PUBACK、PINGREQ/PINGRESP、DISCONNECT 报文编解码。
 * 无需 QtMqtt 附加模块，纯手工实现。
 */

#include "core/icommunication.h"
#include <QTcpSocket>
#include <QTimer>
#include <QList>

class MqttCommunication : public ICommunication
{
    Q_OBJECT

public:
    explicit MqttCommunication(QObject *parent = nullptr);
    ~MqttCommunication() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    bool sendData(const QByteArray &data) override;

    // MQTT 专用方法
    bool subscribe(const QString &topic, int qos = 0);
    bool unsubscribe(const QString &topic);
    bool publish(const QString &topic, const QByteArray &payload, int qos = 0);
    bool ping();

    enum class MqttState {
        Disconnected,
        Connecting,
        SendConnect,
        Connected,
        Subscribing,
        Subscribed
    };
    Q_ENUM(MqttState)
    MqttState mqttState() const { return m_mqttState; }

signals:
    void messageReceived(const QString &topic, const QByteArray &payload);
    void connackReceived(int returnCode);
    void subackReceived(int packetId, const QList<int> &grantedQos);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onPingTimeout();

public:
    // MQTT 3.1.1 报文构建 (public for testability)
    QByteArray buildConnectPacket() const;
    QByteArray buildSubscribePacket(const QString &topic, int qos);
    QByteArray buildUnsubscribePacket(const QString &topic);
    QByteArray buildPublishPacket(const QString &topic, const QByteArray &payload, int qos);
    QByteArray buildPingreqPacket() const;
    QByteArray buildDisconnectPacket() const;
    QByteArray buildPubackPacket(int packetId) const;

    // MQTT 3.1.1 报文解析 (public for testability)
    void parseIncomingData();
    bool parseConnack(const QByteArray &data);
    bool parseSuback(const QByteArray &data);
    bool parsePublish(quint8 firstByte, const QByteArray &remainingData);
    bool parsePingresp();

    // 工具方法 (public static for testability and reuse)
    static QByteArray encodeRemainingLength(int length);
    static int decodeRemainingLength(const QByteArray &data, int *bytesConsumed);
    static QByteArray encodeString(const QString &str);
    static QString decodeString(const QByteArray &data, int offset, int *bytesConsumed);

private:
    QTcpSocket *m_socket = nullptr;
    QTimer *m_pingTimer = nullptr;
    MqttState m_mqttState = MqttState::Disconnected;
    int m_packetId = 1;
    QByteArray m_receiveBuffer;
    bool m_suppressDisconnectSignal = false;  // 防止 abort() 触发的 disconnected 信号干扰新连接
};

#endif // DEBUGPLATFORM_MQTTCOMMUNICATION_H