/**
 * @file mqttcommunication.cpp
 * @brief MQTT 3.1.1 客户端通信实现（纯手写协议栈，基于 QTcpSocket）
 *
 * 支持：CONNECT/CONNACK / SUBSCRIBE/SUBACK / PUBLISH/PUBACK /
 *        PINGREQ/PINGRESP / UNSUBSCRIBE/UNSUBACK / DISCONNECT
 *
 * 诊断日志（双路）：
 *   A. fprintf(stderr, ...) + fflush -> Qt Creator "应用程序输出"面板（100% 可达）
 *   B. LogManager::instance().log(...) -> UI 日志面板（绕开 qInstallMessageHandler 的问题）
 */
#include "core/mqttcommunication.h"
#include "core/config.h"
#include "data/logmanager.h"

#include <QTcpSocket>
#include <QTimer>
#include <QDateTime>
#include <QHostAddress>
#include <cstdio>

static const quint8 CONNECT    = 1;
static const quint8 CONNACK    = 2;
static const quint8 PUBLISH    = 3;
static const quint8 PUBACK     = 4;
static const quint8 SUBSCRIBE  = 8;
static const quint8 SUBACK     = 9;
static const quint8 UNSUBSCRIBE= 10;
static const quint8 UNSUBACK   = 11;
static const quint8 PINGREQ    = 12;
static const quint8 PINGRESP   = 13;
static const quint8 DISCONNECT = 14;

#define MQTT_LOG(level, tag, streamExpr) do { \
    QString _msg; QDebug _d(&_msg); _d.nospace() << streamExpr; \
    const QByteArray _u = _msg.toUtf8(); \
    ::fprintf(stderr, "[MQTT " tag "] %s\n", _u.constData()); \
    ::fflush(stderr); \
    LogManager::instance().log(LogLevel::level, QStringLiteral("mqtt"), _msg); \
} while (false)

MqttCommunication::MqttCommunication(QObject *parent)
    : ICommunication(parent)
{
    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(30000);
    connect(m_pingTimer, &QTimer::timeout, this, &MqttCommunication::onPingTimeout);
}

MqttCommunication::~MqttCommunication()
{
    close();
}

bool MqttCommunication::isOpen() const
{
    return m_isOpen && m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool MqttCommunication::sendData(const QByteArray &data)
{
    const QString &topic = m_config.mqtt.publishTopic.isEmpty()
                               ? QStringLiteral("debug/platform/DebugPlatform/cmd")
                               : m_config.mqtt.publishTopic;
    return publish(topic, data, 0);
}

bool MqttCommunication::open()
{
    if (m_isOpen) return true;

    const MqttConfig &cfg = m_config.mqtt;

    if (cfg.brokerHost.isEmpty() || cfg.brokerPort == 0) {
        emit errorOccurred(tr("MQTT 配置无效: broker=%1 port=%2")
                           .arg(cfg.brokerHost).arg(cfg.brokerPort), -1);
        return false;
    }

    if (!m_socket) {
        m_socket = new QTcpSocket(this);
        connect(m_socket, &QTcpSocket::connected,    this, &MqttCommunication::onSocketConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &MqttCommunication::onSocketDisconnected);
        connect(m_socket, &QTcpSocket::readyRead,    this, &MqttCommunication::onSocketReadyRead);
        connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    }

    m_mqttState = MqttState::Connecting;
    m_receiveBuffer.clear();

    // 修复竞态：只在 socket 确实处于连接/连接中状态时才 abort，
    // 避免无条件 abort() 触发的 disconnected() 信号覆盖新连接状态
    m_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_suppressDisconnectSignal = true;  // 屏蔽 abort() 导致的断开信号
        m_socket->abort();
        if (!m_socket->waitForDisconnected(500)) {
            m_socket->abort();
        }
        m_suppressDisconnectSignal = false;  // 恢复
    }
    m_socket->connectToHost(cfg.brokerHost, cfg.brokerPort);

    const int timeout = Constants::MQTT_CONNECT_TIMEOUT;
    if (!m_socket->waitForConnected(timeout)) {
        emit errorOccurred(tr("MQTT TCP 连接失败: %1:%2 - %3")
                           .arg(cfg.brokerHost).arg(cfg.brokerPort)
                           .arg(m_socket->errorString()),
                           static_cast<int>(m_socket->error()));
        m_mqttState = MqttState::Disconnected;
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_suppressDisconnectSignal = true;
            m_socket->abort();
            m_suppressDisconnectSignal = false;
        }
        return false;
    }

    m_mqttState = MqttState::SendConnect;
    const QByteArray connectPacket = buildConnectPacket();
    const qint64 written = m_socket->write(connectPacket);
    m_socket->flush();
    MQTT_LOG(Info, "CONNECT", "send -> " << cfg.brokerHost << ":" << cfg.brokerPort
             << "  " << written << " bytes  hex="
             << QString::fromLatin1(connectPacket.toHex(' ').toUpper()));

    if (!m_socket->waitForReadyRead(timeout)) {
        MQTT_LOG(Error, "CONNACK", "timeout (" << timeout << "ms) no response from broker");
        emit errorOccurred(tr("MQTT CONNACK 超时 (%1ms)").arg(timeout), -2);
        m_mqttState = MqttState::Disconnected;
        m_suppressDisconnectSignal = true;
        m_socket->abort();
        m_suppressDisconnectSignal = false;
        return false;
    }

    onSocketReadyRead();

    if (m_mqttState != MqttState::Connected) {
        emit errorOccurred(tr("MQTT 连接被 Broker 拒绝"), -3);
        m_mqttState = MqttState::Disconnected;
        m_suppressDisconnectSignal = true;
        m_socket->abort();
        m_suppressDisconnectSignal = false;
        return false;
    }

    m_isOpen = true;
    emit connected();

    if (!m_config.mqtt.subscribeTopic.isEmpty()) {
        subscribe(m_config.mqtt.subscribeTopic, m_config.mqtt.qos);
        MQTT_LOG(Info, "AUTO-SUB", "auto subscribe: topic=" << m_config.mqtt.subscribeTopic
                 << "  QoS=" << m_config.mqtt.qos);
    }

    const int keepAlive = m_config.mqtt.keepAlive > 0 ? m_config.mqtt.keepAlive
                                                       : Constants::MQTT_DEFAULT_KEEP_ALIVE;
    const int pingInterval = qMax(keepAlive * 500, 10000);
    m_pingTimer->setInterval(pingInterval);
    m_pingTimer->start();
    MQTT_LOG(Info, "OPEN", "done. keepAlive=" << keepAlive << "s  pingInterval="
             << pingInterval << "ms");

    return true;
}

void MqttCommunication::close()
{
    if (!m_isOpen && m_mqttState == MqttState::Disconnected &&
        (!m_socket || m_socket->state() == QAbstractSocket::UnconnectedState)) {
        return;
    }

    m_pingTimer->stop();
    m_suppressDisconnectSignal = true;  // 屏蔽 close() 过程中的断开信号

    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        const QByteArray disconnect = buildDisconnectPacket();
        m_socket->write(disconnect);
        m_socket->flush();
        m_socket->waitForBytesWritten(500);
        MQTT_LOG(Info, "CLOSE", "sent DISCONNECT 0xE0 0x00");
    }

    if (m_socket) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(1500);
        }
    }

    m_isOpen = false;
    m_mqttState = MqttState::Disconnected;
    m_receiveBuffer.clear();
    m_suppressDisconnectSignal = false;  // 恢复
    MQTT_LOG(Info, "CLOSE", "resources released");
}

bool MqttCommunication::subscribe(const QString &topic, int qos)
{
    if (m_mqttState < MqttState::Connected) {
        emit errorOccurred(tr("MQTT 未连接，无法订阅"), -4);
        return false;
    }

    const QByteArray packet = buildSubscribePacket(topic, qos);
    m_mqttState = MqttState::Subscribing;
    const qint64 written = m_socket->write(packet);
    m_socket->flush();
    MQTT_LOG(Info, "SUBSCRIBE", "send topic=" << topic << "  QoS=" << qos
             << "  packet=" << written << " bytes  hex="
             << QString::fromLatin1(packet.toHex(' ').toUpper()));
    return written > 0;
}

bool MqttCommunication::unsubscribe(const QString &topic)
{
    if (m_mqttState < MqttState::Connected) return false;
    const QByteArray packet = buildUnsubscribePacket(topic);
    const qint64 written = m_socket->write(packet);
    m_socket->flush();
    MQTT_LOG(Info, "UNSUBSCRIBE", "topic=" << topic << "  " << written << " bytes");
    return written > 0;
}

bool MqttCommunication::publish(const QString &topic, const QByteArray &payload, int qos)
{
    if (m_mqttState < MqttState::Connected) {
        emit errorOccurred(tr("MQTT 未连接，无法发布"), -5);
        return false;
    }
    if (topic.isEmpty()) {
        emit errorOccurred(tr("MQTT 发布主题为空"), -6);
        return false;
    }
    const QByteArray packet = buildPublishPacket(topic, payload, qos);
    const qint64 written = m_socket->write(packet);
    m_socket->flush();
    MQTT_LOG(Info, "PUB-OUT", "topic=" << topic << "  QoS=" << qos
             << "  payloadLen=" << payload.size() << "  packet=" << written << " bytes");
    return written > 0;
}

bool MqttCommunication::ping()
{
    if (m_mqttState < MqttState::Connected) return false;
    const QByteArray packet = buildPingreqPacket();
    const qint64 written = m_socket->write(packet);
    m_socket->flush();
    MQTT_LOG(Info, "PINGREQ", "sent " << written << " bytes");
    return written > 0;
}

void MqttCommunication::onSocketConnected()
{
    // open() 同步模式已处理 CONNECT 发送，这里避免 MQTT 双 CONNECT 违例
    MQTT_LOG(Info, "SOCK", "TCP connected (CONNECT handled in open())");
}

void MqttCommunication::onSocketDisconnected()
{
    m_mqttState = MqttState::Disconnected;
    m_isOpen = false;
    m_pingTimer->stop();
    m_receiveBuffer.clear();
    MQTT_LOG(Warning, "SOCK", "TCP disconnected");
    // 若处于 suppress 模式（open() 内部 abort() 触发），
    // 不对外发出 disconnected() 信号，避免干扰新连接状态
    if (!m_suppressDisconnectSignal) {
        emit disconnected();
    } else {
        MQTT_LOG(Info, "SOCK", "suppressed disconnected signal during open/reconnect");
    }
}

void MqttCommunication::onSocketReadyRead()
{
    const QByteArray data = m_socket->readAll();
    MQTT_LOG(Info, "RX", "TCP recv " << data.size() << " bytes  hex="
         << QString::fromLatin1(data.left(200).toHex(' ').toUpper()));
    m_receiveBuffer.append(data);
    parseIncomingData();
}

void MqttCommunication::onSocketError(QAbstractSocket::SocketError error)
{
    if (m_socket) {
        // 如果处于 suppress 模式（open() 内部 abort() 触发的错误），
        // 不对外发出 errorOccurred 信号，避免干扰新连接状态
        if (m_suppressDisconnectSignal) {
            MQTT_LOG(Info, "SOCK-ERR", "suppressed socket error during open/reconnect: err="
                     << static_cast<int>(error) << "(" << error << ")");
            return;
        }
        MQTT_LOG(Warning, "SOCK-ERR", "Socket error=" << static_cast<int>(error)
                 << "(" << error << ")  desc=" << m_socket->errorString());
        emit errorOccurred(tr("MQTT Socket 错误: %1").arg(m_socket->errorString()),
                           static_cast<int>(error));
    }
}

void MqttCommunication::onPingTimeout()
{
    if (m_mqttState >= MqttState::Connected) {
        ping();
    }
}

/* -------- encode helpers -------- */
QByteArray MqttCommunication::encodeRemainingLength(int length)
{
    QByteArray out;
    int encodedByte;
    do {
        encodedByte = length % 128;
        length = length / 128;
        if (length > 0) encodedByte |= 0x80;
        out.append(static_cast<char>(encodedByte));
    } while (length > 0);
    return out;
}

int MqttCommunication::decodeRemainingLength(const QByteArray &data, int *bytesConsumed)
{
    int multiplier = 1, value = 0, index = 0, encodedByte;
    do {
        if (index >= data.size()) { if (bytesConsumed) *bytesConsumed = 0; return -1; }
        encodedByte = static_cast<quint8>(data.at(index));
        value += (encodedByte & 0x7F) * multiplier;
        multiplier *= 128;
        if (multiplier > 128*128*128) { if (bytesConsumed) *bytesConsumed = 0; return -1; }
        ++index;
    } while ((encodedByte & 0x80) != 0);
    if (bytesConsumed) *bytesConsumed = index;
    return value;
}

QByteArray MqttCommunication::encodeString(const QString &str)
{
    const QByteArray utf8 = str.toUtf8();
    QByteArray out;
    const quint16 len = static_cast<quint16>(utf8.size());
    out.append(static_cast<char>((len >> 8) & 0xFF));
    out.append(static_cast<char>(len & 0xFF));
    out.append(utf8);
    return out;
}

QString MqttCommunication::decodeString(const QByteArray &data, int offset, int *bytesConsumed)
{
    if (offset + 2 > data.size()) { if (bytesConsumed) *bytesConsumed = 0; return QString(); }
    const quint16 len = (static_cast<quint8>(data.at(offset)) << 8)
                      |  static_cast<quint8>(data.at(offset + 1));
    if (offset + 2 + len > data.size()) { if (bytesConsumed) *bytesConsumed = 0; return QString(); }
    QString result = QString::fromUtf8(data.mid(offset + 2, len));
    if (bytesConsumed) *bytesConsumed = 2 + len;
    return result;
}

/* -------- packet builders -------- */
QByteArray MqttCommunication::buildConnectPacket() const
{
    const MqttConfig &cfg = m_config.mqtt;

    QByteArray payload;
    const QString clientId = cfg.clientId.isEmpty()
        ? QString("DebugPlatform_%1").arg(QDateTime::currentMSecsSinceEpoch() % 100000)
        : cfg.clientId;
    payload.append(encodeString(clientId));
    if (!cfg.username.isEmpty()) payload.append(encodeString(cfg.username));
    if (!cfg.password.isEmpty()) payload.append(encodeString(cfg.password));

    QByteArray varHeader;
    varHeader.append(encodeString(QString("MQTT")));
    varHeader.append(static_cast<char>(Constants::MQTT_PROTOCOL_LEVEL));
    quint8 flags = 0;
    if (cfg.cleanSession) flags |= 0x02;
    if (!cfg.username.isEmpty()) flags |= 0x80;
    if (!cfg.password.isEmpty()) flags |= 0x40;
    varHeader.append(static_cast<char>(flags));
    const int keepAlive = cfg.keepAlive > 0 ? cfg.keepAlive : Constants::MQTT_DEFAULT_KEEP_ALIVE;
    varHeader.append(static_cast<char>((keepAlive >> 8) & 0xFF));
    varHeader.append(static_cast<char>(keepAlive & 0xFF));

    const int remainingLength = varHeader.size() + payload.size();
    QByteArray packet;
    packet.append(static_cast<char>(0x10));
    packet.append(encodeRemainingLength(remainingLength));
    packet.append(varHeader);
    packet.append(payload);
    return packet;
}

QByteArray MqttCommunication::buildSubscribePacket(const QString &topic, int qos)
{
    QByteArray payload;
    payload.append(encodeString(topic));
    payload.append(static_cast<char>(qos & 0x03));

    const int pid = m_packetId++;
    if (m_packetId > 0xFFFF) m_packetId = 1;
    QByteArray varHeader;
    varHeader.append(static_cast<char>((pid >> 8) & 0xFF));
    varHeader.append(static_cast<char>(pid & 0xFF));

    const int remainingLength = varHeader.size() + payload.size();
    QByteArray packet;
    packet.append(static_cast<char>(0x82));
    packet.append(encodeRemainingLength(remainingLength));
    packet.append(varHeader);
    packet.append(payload);
    return packet;
}

QByteArray MqttCommunication::buildUnsubscribePacket(const QString &topic)
{
    QByteArray payload = encodeString(topic);
    const int pid = m_packetId++;
    if (m_packetId > 0xFFFF) m_packetId = 1;
    QByteArray varHeader;
    varHeader.append(static_cast<char>((pid >> 8) & 0xFF));
    varHeader.append(static_cast<char>(pid & 0xFF));
    const int remainingLength = varHeader.size() + payload.size();
    QByteArray packet;
    packet.append(static_cast<char>(0xA2));
    packet.append(encodeRemainingLength(remainingLength));
    packet.append(varHeader);
    packet.append(payload);
    return packet;
}

QByteArray MqttCommunication::buildPublishPacket(const QString &topic, const QByteArray &payload, int qos)
{
    QByteArray varHeader = encodeString(topic);
    const bool hasPacketId = (qos >= 1);
    if (hasPacketId) {
        const int pid = m_packetId++;
        if (m_packetId > 0xFFFF) m_packetId = 1;
        varHeader.append(static_cast<char>((pid >> 8) & 0xFF));
        varHeader.append(static_cast<char>(pid & 0xFF));
    }
    const int remainingLength = varHeader.size() + payload.size();
    QByteArray packet;
    quint8 firstByte = 0x30;
    firstByte |= static_cast<quint8>((qos & 0x03) << 1);
    packet.append(static_cast<char>(firstByte));
    packet.append(encodeRemainingLength(remainingLength));
    packet.append(varHeader);
    packet.append(payload);
    return packet;
}

QByteArray MqttCommunication::buildPingreqPacket() const
{
    QByteArray p; p.append(static_cast<char>(0xC0)); p.append(static_cast<char>(0x00)); return p;
}

QByteArray MqttCommunication::buildDisconnectPacket() const
{
    QByteArray p; p.append(static_cast<char>(0xE0)); p.append(static_cast<char>(0x00)); return p;
}

QByteArray MqttCommunication::buildPubackPacket(int packetId) const
{
    QByteArray p;
    p.append(static_cast<char>(0x40));
    p.append(static_cast<char>(0x02));
    p.append(static_cast<char>((packetId >> 8) & 0xFF));
    p.append(static_cast<char>(packetId & 0xFF));
    return p;
}

/* -------- parsers -------- */

void MqttCommunication::parseIncomingData()
{
    while (m_receiveBuffer.size() >= 2) {
        const quint8 firstByte = static_cast<quint8>(m_receiveBuffer.at(0));
        const int msgType = (firstByte >> 4) & 0x0F;

        int rlBytesConsumed = 0;
        const int remainingLength = decodeRemainingLength(
            m_receiveBuffer.mid(1), &rlBytesConsumed);
        if (remainingLength < 0) {
            MQTT_LOG(Warning, "PARSE", "remaining length incomplete (buffer="
                     << m_receiveBuffer.size() << ")");
            return;
        }
        const int totalSize = 1 + rlBytesConsumed + remainingLength;
        if (m_receiveBuffer.size() < totalSize) {
            MQTT_LOG(Info, "PARSE", "packet fragmented: need " << totalSize
                     << " have " << m_receiveBuffer.size() << ", waiting");
            return;
        }
        const QByteArray remainingData = m_receiveBuffer.mid(1 + rlBytesConsumed, remainingLength);
        m_receiveBuffer.remove(0, totalSize);

        switch (msgType) {
        case CONNACK: {
            MQTT_LOG(Info, "PKT", "type=CONNACK rl=" << remainingLength
                     << " hex=" << QString::fromLatin1(remainingData.toHex(' ').toUpper()));
            parseConnack(remainingData); break;
        }
        case PUBLISH: {
            MQTT_LOG(Info, "PKT", "type=PUBLISH rl=" << remainingLength
                     << " firstByte=0x" << QString::number(firstByte, 16).toUpper());
            parsePublish(firstByte, remainingData); break;
        }
        case PUBACK: {
            MQTT_LOG(Info, "PKT", "type=PUBACK rl=" << remainingLength << " (ack)"); break;
        }
        case SUBACK: {
            MQTT_LOG(Info, "PKT", "type=SUBACK rl=" << remainingLength
                     << " hex=" << QString::fromLatin1(remainingData.toHex(' ').toUpper()));
            parseSuback(remainingData); break;
        }
        case PINGRESP: {
            MQTT_LOG(Info, "PKT", "type=PINGRESP rl=" << remainingLength);
            parsePingresp(); break;
        }
        case UNSUBACK: {
            MQTT_LOG(Info, "PKT", "type=UNSUBACK rl=" << remainingLength); break;
        }
        default: {
            MQTT_LOG(Warning, "PKT", "Unknown type=" << msgType
                     << " firstByte=0x" << QString::number(firstByte, 16).toUpper());
            break;
        }
        }
    }
}

bool MqttCommunication::parseConnack(const QByteArray &data)
{
    if (data.size() < 2) return false;
    const int returnCode = static_cast<int>(static_cast<quint8>(data.at(1)));
    emit connackReceived(returnCode);

    if (returnCode == 0) {
        m_mqttState = MqttState::Connected;
        MQTT_LOG(Info, "CONNACK", "rc=0 握手成功 session_present="
                 << static_cast<int>(static_cast<quint8>(data.at(0))));
        return true;
    } else {
        m_mqttState = MqttState::Disconnected;
        MQTT_LOG(Error, "CONNACK", "rc=" << returnCode
                 << " Broker 拒绝（1=协议版本 2=clientId 3=不可用 4=账号密码坏 5=未授权）");
        emit errorOccurred(tr("MQTT CONNACK 错误码: %1").arg(returnCode), -10);
        return false;
    }
}

bool MqttCommunication::parseSuback(const QByteArray &data)
{
    if (data.size() < 3) return false;
    const int packetId = (static_cast<quint8>(data.at(0)) << 8)
                       |  static_cast<quint8>(data.at(1));
    QList<int> grantedQos;
    for (int i = 2; i < data.size(); ++i)
        grantedQos.append(static_cast<int>(static_cast<quint8>(data.at(i)) & 0x03));
    emit subackReceived(packetId, grantedQos);
    m_mqttState = MqttState::Subscribed;
    QStringList qs;
    for (int q : grantedQos) qs << QString::number(q);
    MQTT_LOG(Info, "SUBACK", "订阅成功 packetId=" << packetId
             << " grantedQoS=[" << qs.join(", ") << "]");
    return true;
}

bool MqttCommunication::parsePublish(quint8 firstByte, const QByteArray &remainingData)
{
    const int qos = (firstByte >> 1) & 0x03;

    int offset = 0, consumed = 0;
    const QString topic = decodeString(remainingData, offset, &consumed);
    if (consumed == 0 || topic.isEmpty()) {
        MQTT_LOG(Warning, "PUB-IN", "topic decode failed, remainingData len="
                 << remainingData.size());
        return false;
    }
    offset += consumed;

    if (qos >= 1) {
        if (offset + 2 > remainingData.size()) {
            MQTT_LOG(Warning, "PUB-IN", "QoS=" << qos << " missing packet id");
            return false;
        }
        const int packetId = (static_cast<quint8>(remainingData.at(offset)) << 8)
                           |  static_cast<quint8>(remainingData.at(offset + 1));
        offset += 2;
        if (qos == 1) {
            const QByteArray ack = buildPubackPacket(packetId);
            if (m_socket) { m_socket->write(ack); m_socket->flush(); }
            MQTT_LOG(Info, "PUB-IN", "QoS1 -> sent PUBACK pid=" << packetId);
        }
    }

    const QByteArray payload = remainingData.mid(offset);
    const QByteArray preview = payload.size() > 120 ? payload.left(120) + "..." : payload;
    MQTT_LOG(Info, "PUB-IN", "topic=" << topic
             << " QoS=" << qos
             << " payloadLen=" << payload.size()
             << " preview=" << QString::fromUtf8(preview));

    emit messageReceived(topic, payload);
    return true;
}

bool MqttCommunication::parsePingresp()
{
    MQTT_LOG(Info, "PINGRESP", "heartbeat ok");
    return true;
}
