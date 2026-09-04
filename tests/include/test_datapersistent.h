#ifndef TEST_DATAPERSISTENT_H
#define TEST_DATAPERSISTENT_H

#include <QObject>
#include <QtTest/QtTest>
#include <QCoreApplication>
#include "data/datapersistent.h"

class TestDataPersistent : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();
    
    // DataPersistent 基础测试
    void testInitialize();
    void testInsertRegisterData();
    void testQueryRegisterData();
    void testQueryByDeviceId();
    void testQueryByTimeRange();
    void testCleanupOldData();
    void testConfigSaveLoad();
    
    // 设备管理测试
    void testAddDevice();
    void testAddDeviceConflict();
    void testRemoveDevice();
    void testLoadAllDevices();
    
    // 批量数据测试
    void testInsertRegisterDataBatch();
    void testInsertRegisterDataBatchEmpty();
    
    // 告警测试
    void testInsertAlert();
    void testQueryAlerts();
    void testQueryRecentAlerts();
    
    // 错误处理测试
    void testNotInitialized();
    void testDoubleInitialize();
    void testCleanupEdgeCases();
    void testConfigNonexistent();

    // P1-3a 错误路径测试
    void testDatabaseClosed();
    void testForeignKeyViolation();
    void testQueryNoResults();
    
private:
    QString m_testDbPath;
    DataPersistent* m_db;
};

#endif // TEST_DATAPERSISTENT_H
