#ifndef TEST_MQTTPROTOCOL_H
#define TEST_MQTTPROTOCOL_H

#include <QObject>
#include "protocol/mqttprotocol.h"

class TestMqttProtocol : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void testParseSinglePoint();
    void testParseArray();
    void testParseSimpleValue();
    void testParseInvalidJson();
    void testParseWithDeviceId();
    void testExtractDeviceId();
    void testExtractDeviceIdSimple();
    void testExtractAddress();
    void testExtractAddressNotFound();
    void testBuildJsonPayloadSingle();
    void testBuildJsonPayloadArray();
    void testBuildAndParseRoundTrip();
};

#endif // TEST_MQTTPROTOCOL_H
