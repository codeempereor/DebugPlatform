#include "test_mqttprotocol.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtTest>
#include <QDebug>

void TestMqttProtocol::initTestCase()
{
    qDebug() << "=== TestMqttProtocol 启动 ===";
}

void TestMqttProtocol::cleanupTestCase()
{
    qDebug() << "=== TestMqttProtocol 结束 ===";
}

void TestMqttProtocol::testParseSinglePoint()
{
    // {"addr": 5, "value": 123.45}
    QByteArray json = QJsonDocument(QJsonObject{{"addr", 5}, {"value", 123.45}})
                          .toJson(QJsonDocument::Compact);

    QList<MqttDataPoint> points = MqttProtocol::parseMessage("devices/dev1/data/5", json);
    QCOMPARE(points.size(), 1);
    QCOMPARE(points[0].address, 5);
    QCOMPARE(points[0].value, 123.45);
    QCOMPARE(points[0].deviceId, QString("dev1"));
}

void TestMqttProtocol::testParseArray()
{
    // {"data": [{"addr": 0, "value": 1.1}, {"addr": 1, "value": 2.2}]}
    QJsonArray arr;
    arr.append(QJsonObject{{"addr", 0}, {"value", 1.1}});
    arr.append(QJsonObject{{"addr", 1}, {"value", 2.2}});
    QByteArray json = QJsonDocument(QJsonObject{{"data", arr}})
                          .toJson(QJsonDocument::Compact);

    QList<MqttDataPoint> points = MqttProtocol::parseMessage("devices/dev1/data", json);
    QCOMPARE(points.size(), 2);
    QCOMPARE(points[0].address, 0);
    QCOMPARE(points[0].value, 1.1);
    QCOMPARE(points[1].address, 1);
    QCOMPARE(points[1].value, 2.2);
}

void TestMqttProtocol::testParseSimpleValue()
{
    // Just "42.5" - pure numeric, addr from topic
    QList<MqttDataPoint> points = MqttProtocol::parseMessage("devices/dev1/data/3", "42.5");
    QCOMPARE(points.size(), 1);
    QCOMPARE(points[0].address, 3);
    QCOMPARE(points[0].value, 42.5);
    QCOMPARE(points[0].deviceId, QString("dev1"));
}

void TestMqttProtocol::testParseInvalidJson()
{
    QList<MqttDataPoint> points = MqttProtocol::parseMessage("devices/dev1/data/0", "not json");
    // Should return empty or parse as numeric (fails -> empty)
    QVERIFY(points.isEmpty());
}

void TestMqttProtocol::testParseWithDeviceId()
{
    // {"deviceId": "custom_dev", "addr": 10, "value": 99.9}
    QByteArray json = QJsonDocument(QJsonObject{
        {"deviceId", "custom_dev"},
        {"addr", 10},
        {"value", 99.9}
    }).toJson(QJsonDocument::Compact);

    QList<MqttDataPoint> points = MqttProtocol::parseMessage("devices/default_dev/data/10", json);
    QCOMPARE(points.size(), 1);
    QCOMPARE(points[0].deviceId, QString("custom_dev"));
    QCOMPARE(points[0].address, 10);
    QCOMPARE(points[0].value, 99.9);
}

void TestMqttProtocol::testExtractDeviceId()
{
    QCOMPARE(MqttProtocol::extractDeviceId("devices/sensor1/data/0"), QString("sensor1"));
    QCOMPARE(MqttProtocol::extractDeviceId("devices/my_device/data"), QString("my_device"));
}

void TestMqttProtocol::testExtractDeviceIdSimple()
{
    // Format: {deviceId}/data/...
    QCOMPARE(MqttProtocol::extractDeviceId("sensor1/data/0"), QString("sensor1"));
}

void TestMqttProtocol::testExtractAddress()
{
    QCOMPARE(MqttProtocol::extractAddress("devices/dev1/data/5"), 5);
    QCOMPARE(MqttProtocol::extractAddress("devices/dev1/data/100"), 100);
}

void TestMqttProtocol::testExtractAddressNotFound()
{
    QCOMPARE(MqttProtocol::extractAddress("devices/dev1/status"), -1);
    QCOMPARE(MqttProtocol::extractAddress("no/data/here"), -1);
}

void TestMqttProtocol::testBuildJsonPayloadSingle()
{
    QByteArray json = MqttProtocol::buildJsonPayload(5, 123.45);
    QJsonDocument doc = QJsonDocument::fromJson(json);
    QVERIFY(doc.isObject());
    QCOMPARE(doc.object().value("addr").toInt(), 5);
    QCOMPARE(doc.object().value("value").toDouble(), 123.45);
}

void TestMqttProtocol::testBuildJsonPayloadArray()
{
    QList<QPair<int, double>> points;
    points.append(qMakePair(0, 1.1));
    points.append(qMakePair(1, 2.2));

    QByteArray json = MqttProtocol::buildJsonPayload(points);
    QJsonDocument doc = QJsonDocument::fromJson(json);
    QVERIFY(doc.isObject());
    QVERIFY(doc.object().value("data").isArray());
    QJsonArray arr = doc.object().value("data").toArray();
    QCOMPARE(arr.size(), 2);
    QCOMPARE(arr.at(0).toObject().value("addr").toInt(), 0);
    QCOMPARE(arr.at(0).toObject().value("value").toDouble(), 1.1);
}

void TestMqttProtocol::testBuildAndParseRoundTrip()
{
    // Build single point
    QByteArray json = MqttProtocol::buildJsonPayload(42, 3.14159);
    QList<MqttDataPoint> points = MqttProtocol::parseMessage("devices/test/data/42", json);
    QCOMPARE(points.size(), 1);
    QCOMPARE(points[0].address, 42);
    QCOMPARE(points[0].value, 3.14159);

    // Build array
    QList<QPair<int, double>> input;
    for (int i = 0; i < 5; ++i) {
        input.append(qMakePair(i, i * 10.5));
    }
    QByteArray jsonArr = MqttProtocol::buildJsonPayload(input);
    QList<MqttDataPoint> result = MqttProtocol::parseMessage("devices/test/data", jsonArr);
    QCOMPARE(result.size(), 5);
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(result[i].address, i);
        QCOMPARE(result[i].value, i * 10.5);
    }
}
