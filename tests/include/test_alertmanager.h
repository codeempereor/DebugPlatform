#ifndef TEST_ALERTMANAGER_H
#define TEST_ALERTMANAGER_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "data/alertmanager.h"

class TestAlertManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // AlertManager 基础测试
    void testAddAlertConfig();
    void testCheckValueWithinRange();
    void testCheckValueBelowMin();
    void testCheckValueAboveMax();
    void testCheckValueDisabled();
    void testRemoveAlertConfig();
    void testClearDeviceConfigs();
    void testGetAlertHistory();
    
private:
    AlertManager m_alertManager;
};

#endif // TEST_ALERTMANAGER_H
