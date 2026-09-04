#include "test_mqttcommunication.h"
#include "core/communicationfactory.h"
#include "core/communication_config.h"
#include <QSignalSpy>
#include <QTest>

void TestMqttCommunication::initTestCase()
{
    m_comm = new MqttCommunication(this);
    qDebug() << "=== TestMqttCommunication 启动 ===";
}

void TestMqttCommunication::cleanupTestCase()
{
    if (m_comm) delete m_comm;
    qDebug() << "=== TestMqttCommunication 结束 ===";
}

// --- Remaining Length ---

void TestMqttCommunication::testEncodeRemainingLengthSmall()
{
    // 0-127 should be 1 byte
    QByteArray r0 = MqttCommunication::encodeRemainingLength(0);
    QCOMPARE(r0.size(), 1);
    QCOMPARE((quint8)r0.at(0), 0x00);

    QByteArray r127 = MqttCommunication::encodeRemainingLength(127);
    QCOMPARE(r127.size(), 1);
    QCOMPARE((quint8)r127.at(0), 0x7F);
}

void TestMqttCommunication::testEncodeRemainingLengthMedium()
{
    // 128-16383 should be 2 bytes
    QByteArray r128 = MqttCommunication::encodeRemainingLength(128);
    QCOMPARE(r128.size(), 2);
    QCOMPARE((quint8)r128.at(0), (quint8)0x80);
    QCOMPARE((quint8)r128.at(1), (quint8)0x01);

    QByteArray r16383 = MqttCommunication::encodeRemainingLength(16383);
    QCOMPARE(r16383.size(), 2);
    QCOMPARE((quint8)r16383.at(0), (quint8)0xFF);
    QCOMPARE((quint8)r16383.at(1), (quint8)0x7F);
}

void TestMqttCommunication::testEncodeRemainingLengthLarge()
{
    // 16384+ should be 3 bytes
    QByteArray r16384 = MqttCommunication::encodeRemainingLength(16384);
    QCOMPARE(r16384.size(), 3);
    QCOMPARE((quint8)r16384.at(0), (quint8)0x80);
    QCOMPARE((quint8)r16384.at(1), (quint8)0x80);
    QCOMPARE((quint8)r16384.at(2), (quint8)0x01);
}

void TestMqttCommunication::testDecodeRemainingLengthRoundTrip()
{
    int testValues[] = {0, 1, 127, 128, 255, 16383, 16384, 100000, 2097151};
    for (int val : testValues) {
        QByteArray encoded = MqttCommunication::encodeRemainingLength(val);
        int consumed = 0;
        int decoded = MqttCommunication::decodeRemainingLength(encoded, &consumed);
        QCOMPARE(decoded, val);
        QCOMPARE(consumed, encoded.size());
    }
}

// --- String Encoding ---

void TestMqttCommunication::testEncodeStringAscii()
{
    QByteArray encoded = MqttCommunication::encodeString("hello");
    // 2 bytes length prefix (big-endian) + 5 bytes content
    QCOMPARE(encoded.size(), 7);
    QCOMPARE((quint8)encoded.at(0), 0x00);
    QCOMPARE((quint8)encoded.at(1), 0x05);
    QCOMPARE(encoded.mid(2), QByteArray("hello"));
}

void TestMqttCommunication::testEncodeStringUtf8()
{
    QByteArray encoded = MqttCommunication::encodeString(QString::fromUtf8("测试"));
    // "测试" = 6 bytes in UTF-8, + 2 bytes length = 8
    QCOMPARE(encoded.size(), 8);
    QCOMPARE((quint8)encoded.at(0), 0x00);
    QCOMPARE((quint8)encoded.at(1), 0x06);
    QCOMPARE(encoded.mid(2), QString::fromUtf8("测试").toUtf8());
}

void TestMqttCommunication::testEncodeStringEmpty()
{
    QByteArray encoded = MqttCommunication::encodeString("");
    QCOMPARE(encoded.size(), 2);
    QCOMPARE((quint8)encoded.at(0), 0x00);
    QCOMPARE((quint8)encoded.at(1), 0x00);
}

void TestMqttCommunication::testDecodeStringRoundTrip()
{
    QStringList testStrings;
    testStrings << "" << "hello" << "world123"
                << QString::fromUtf8("测试") << "devices/sensor1/data";

    for (const QString &str : testStrings) {
        QByteArray encoded = MqttCommunication::encodeString(str);
        int consumed = 0;
        QString decoded = MqttCommunication::decodeString(encoded, 0, &consumed);
        QCOMPARE(decoded, str);
        QCOMPARE(consumed, encoded.size());
    }
}

// --- Packet Building ---

void TestMqttCommunication::testBuildConnectPacket()
{
    CommunicationConfig config;
    config.type = CommunicationType::Mqtt;
    config.mqtt.brokerHost = "127.0.0.1";
    config.mqtt.brokerPort = 1883;
    config.mqtt.clientId = "test_client";
    config.mqtt.keepAlive = 60;
    config.mqtt.cleanSession = true;
    m_comm->setConfig(config);

    QByteArray packet = m_comm->buildConnectPacket();

    // Fixed header: 0x10 (CONNECT) + remaining length
    QCOMPARE((quint8)packet.at(0), (quint8)0x10);

    // Protocol name "MQTT" after 2-byte length prefix
    int offset = 1;
    // Skip remaining length (1 byte for small packets)
    offset += 1;  // assume remaining length is 1 byte

    // Protocol Name: 0x00 0x04 "MQTT"
    QCOMPARE((quint8)packet.at(offset), 0x00);
    QCOMPARE((quint8)packet.at(offset+1), 0x04);
    QCOMPARE(packet.mid(offset+2, 4), QByteArray("MQTT"));

    // Protocol Level: 4 (MQTT 3.1.1)
    QCOMPARE((quint8)packet.at(offset+6), (quint8)0x04);

    // Connect Flags: Clean Session = 0x02
    quint8 flags = (quint8)packet.at(offset+7);
    QVERIFY(flags & 0x02);  // Clean Session bit set

    // Keep Alive: 60 = 0x00 0x3C
    QCOMPARE((quint8)packet.at(offset+8), 0x00);
    QCOMPARE((quint8)packet.at(offset+9), 0x3C);

    // Verify packet contains "test_client" in payload
    QVERIFY(packet.contains("test_client"));
}

void TestMqttCommunication::testBuildSubscribePacket()
{
    QByteArray packet = m_comm->buildSubscribePacket("devices/sensor1/data", 1);

    // Fixed header: 0x82 (SUBSCRIBE with flags=0x02)
    QCOMPARE((quint8)packet.at(0), (quint8)0x82);

    // Verify topic is in the packet
    QVERIFY(packet.contains("devices/sensor1/data"));

    // Last byte before end should be QoS=1
    QCOMPARE((quint8)packet.at(packet.size()-1), (quint8)0x01);
}

void TestMqttCommunication::testBuildPublishPacketQoS0()
{
    QByteArray payload = QByteArray::fromHex("DEADBEEF");
    QByteArray packet = m_comm->buildPublishPacket("test/topic", payload, 0);

    // Fixed header: 0x30 (PUBLISH, QoS0, no DUP, no RETAIN)
    QCOMPARE((quint8)packet.at(0), (quint8)0x30);

    // Verify topic and payload
    QVERIFY(packet.contains("test/topic"));
    QVERIFY(packet.contains(payload));
}

void TestMqttCommunication::testBuildPublishPacketQoS1()
{
    QByteArray payload = "hello mqtt";
    QByteArray packet = m_comm->buildPublishPacket("test/topic", payload, 1);

    // Fixed header: 0x32 (PUBLISH, QoS1)
    QCOMPARE((quint8)packet.at(0), (quint8)0x32);

    QVERIFY(packet.contains("test/topic"));
    QVERIFY(packet.contains(payload));
}

void TestMqttCommunication::testBuildPingreqPacket()
{
    QByteArray packet = m_comm->buildPingreqPacket();
    QCOMPARE(packet.size(), 2);
    QCOMPARE((quint8)packet.at(0), (quint8)0xC0);
    QCOMPARE((quint8)packet.at(1), (quint8)0x00);
}

void TestMqttCommunication::testBuildDisconnectPacket()
{
    QByteArray packet = m_comm->buildDisconnectPacket();
    QCOMPARE(packet.size(), 2);
    QCOMPARE((quint8)packet.at(0), (quint8)0xE0);
    QCOMPARE((quint8)packet.at(1), (quint8)0x00);
}

void TestMqttCommunication::testBuildPubackPacket()
{
    QByteArray packet = m_comm->buildPubackPacket(42);
    QCOMPARE(packet.size(), 4);
    QCOMPARE((quint8)packet.at(0), (quint8)0x40);
    QCOMPARE((quint8)packet.at(1), (quint8)0x02);
    QCOMPARE((quint8)packet.at(2), (quint8)0x00);
    QCOMPARE((quint8)packet.at(3), (quint8)0x2A);  // 42 = 0x2A
}

// --- Packet Parsing ---

void TestMqttCommunication::testParseConnackSuccess()
{
    QSignalSpy spy(m_comm, &MqttCommunication::connackReceived);

    // CONNACK: Session Present=0, Return Code=0 (accepted)
    QByteArray connack;
    connack.append((char)0x00);  // Session Present
    connack.append((char)0x00);  // Return Code = 0 (accepted)

    QVERIFY(m_comm->parseConnack(connack));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 0);
    QCOMPARE(m_comm->mqttState(), MqttCommunication::MqttState::Connected);
}

void TestMqttCommunication::testParseConnackError()
{
    QSignalSpy spy(m_comm, &MqttCommunication::connackReceived);

    // CONNACK with error code 5 (not authorized)
    QByteArray connack;
    connack.append((char)0x00);
    connack.append((char)0x05);

    QVERIFY(!m_comm->parseConnack(connack));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 5);
}

void TestMqttCommunication::testParseSuback()
{
    QSignalSpy spy(m_comm, &MqttCommunication::subackReceived);

    // SUBACK: Packet ID=1, Return Code=0 (QoS0 granted)
    QByteArray suback;
    suback.append((char)0x00);  // Packet ID high
    suback.append((char)0x01);  // Packet ID low
    suback.append((char)0x00);  // Return code = QoS 0

    QVERIFY(m_comm->parseSuback(suback));
    QCOMPARE(spy.count(), 1);
}

void TestMqttCommunication::testParsePublishQoS0()
{
    QSignalSpy msgSpy(m_comm, &MqttCommunication::messageReceived);

    // Build a PUBLISH packet manually
    // Topic: "test/topic" (10 bytes)
    // Payload: "hello"
    QByteArray topic = MqttCommunication::encodeString("test/topic");
    QByteArray payload = "hello";
    QByteArray remainingData = topic + payload;

    // First byte: 0x30 (PUBLISH, QoS0)
    QVERIFY(m_comm->parsePublish(0x30, remainingData));
    QCOMPARE(msgSpy.count(), 1);

    QList<QVariant> args = msgSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("test/topic"));
    QCOMPARE(args.at(1).toByteArray(), QByteArray("hello"));
}

void TestMqttCommunication::testParsePublishQoS1()
{
    QSignalSpy msgSpy(m_comm, &MqttCommunication::messageReceived);

    // Topic: "sensors/temp" + Packet ID + Payload
    QByteArray topic = MqttCommunication::encodeString("sensors/temp");
    QByteArray packetId;
    packetId.append((char)0x00);
    packetId.append((char)0x05);
    QByteArray payload = "23.5";
    QByteArray remainingData = topic + packetId + payload;

    // First byte: 0x32 (PUBLISH, QoS1)
    QVERIFY(m_comm->parsePublish(0x32, remainingData));
    QCOMPARE(msgSpy.count(), 1);

    QList<QVariant> args = msgSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("sensors/temp"));
    QCOMPARE(args.at(1).toByteArray(), QByteArray("23.5"));
}

void TestMqttCommunication::testParsePingresp()
{
    QVERIFY(m_comm->parsePingresp());
}

void TestMqttCommunication::testFactoryCreateMqtt()
{
    CommunicationConfig config;
    config.type = CommunicationType::Mqtt;
    config.mqtt.brokerHost = "127.0.0.1";
    config.mqtt.brokerPort = 1883;

    ICommunication *comm = CommunicationFactory::create(CommunicationType::Mqtt, config);
    QVERIFY(comm != nullptr);
    QVERIFY(dynamic_cast<MqttCommunication*>(comm) != nullptr);
    delete comm;
}
