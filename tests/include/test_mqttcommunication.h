#ifndef TEST_MQTTCOMMUNICATION_H
#define TEST_MQTTCOMMUNICATION_H

#include <QObject>
#include "core/mqttcommunication.h"

class TestMqttCommunication : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    // Remaining Length encoding/decoding
    void testEncodeRemainingLengthSmall();
    void testEncodeRemainingLengthMedium();
    void testEncodeRemainingLengthLarge();
    void testDecodeRemainingLengthRoundTrip();

    // String encoding/decoding
    void testEncodeStringAscii();
    void testEncodeStringUtf8();
    void testEncodeStringEmpty();
    void testDecodeStringRoundTrip();

    // Packet building
    void testBuildConnectPacket();
    void testBuildSubscribePacket();
    void testBuildPublishPacketQoS0();
    void testBuildPublishPacketQoS1();
    void testBuildPingreqPacket();
    void testBuildDisconnectPacket();
    void testBuildPubackPacket();

    // Packet parsing
    void testParseConnackSuccess();
    void testParseConnackError();
    void testParseSuback();
    void testParsePublishQoS0();
    void testParsePublishQoS1();
    void testParsePingresp();

    // Factory
    void testFactoryCreateMqtt();

private:
    MqttCommunication *m_comm = nullptr;
};

#endif // TEST_MQTTCOMMUNICATION_H
