#ifndef TEST_DATACACHE_H
#define TEST_DATACACHE_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>

class TestDataCache : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // DataCache 基础测试
    void testUpdateAndGetValue();
    void testMultipleDevices();
    void testValueChangedSignal();
    void testDeviceDataCleared();
    void testGetDeviceList();
    void testClearDeviceData();
};

#endif // TEST_DATACACHE_H
