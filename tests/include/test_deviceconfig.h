#ifndef TEST_DEVICECONFIG_H
#define TEST_DEVICECONFIG_H

#include <QObject>
#include <QtTest/QtTest>
#include "device/deviceconfig.h"

class TestDeviceConfig : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // DeviceConfig 基础测试
    void testDefaultConfig();
    void testSetProperties();
    void testToMapAndFromMap();
    void testSerialConfig();
    void testTcpConfig();
    void testUdpConfig();
    void testToString();
};

#endif // TEST_DEVICECONFIG_H
