#include "test_alertmanager.h"

void TestAlertManager::initTestCase()
{
    qRegisterMetaType<Alert>("Alert");
    qDebug() << "=== AlertManager 测试套件开始 ===";
}

void TestAlertManager::cleanupTestCase()
{
    qDebug() << "=== AlertManager 测试套件结束 ===";
}

void TestAlertManager::init()
{
    m_alertManager.clearDeviceConfigs("testDevice");
}

void TestAlertManager::cleanup()
{
    m_alertManager.clearDeviceConfigs("testDevice");
}

void TestAlertManager::testAddAlertConfig()
{
    AlertConfig config;
    config.registerAddr = 0;
    config.minValue = 0.0;
    config.maxValue = 100.0;
    config.enabled = true;
    
    m_alertManager.addAlertConfig("testDevice", config);
    
    auto configs = m_alertManager.allConfigsForDevice("testDevice");
    QCOMPARE(configs.size(), 1);
    QCOMPARE(configs.value(0).registerAddr, static_cast<quint16>(0));
    QCOMPARE(configs.value(0).maxValue, 100.0);
    
    qDebug() << "告警配置添加测试通过";
}

void TestAlertManager::testCheckValueWithinRange()
{
    AlertConfig config;
    config.registerAddr = 0;
    config.minValue = 0.0;
    config.maxValue = 100.0;
    config.enabled = true;
    
    m_alertManager.addAlertConfig("testDevice", config);
    
    QSignalSpy spy(&m_alertManager, &AlertManager::alertTriggered);
    
    // 在范围内的值不应触发告警
    m_alertManager.checkValue("testDevice", 0, 50.0);
    QCOMPARE(spy.count(), 0);
    
    m_alertManager.checkValue("testDevice", 0, 0.0);  // 边界值
    QCOMPARE(spy.count(), 0);
    
    m_alertManager.checkValue("testDevice", 0, 100.0); // 边界值
    QCOMPARE(spy.count(), 0);
    
    qDebug() << "范围内值告警测试通过";
}

void TestAlertManager::testCheckValueBelowMin()
{
    AlertConfig config;
    config.registerAddr = 0;
    config.minValue = 10.0;
    config.maxValue = 100.0;
    config.enabled = true;
    
    m_alertManager.addAlertConfig("testDevice", config);
    
    QSignalSpy spy(&m_alertManager, &AlertManager::alertTriggered);
    
    // 低于最小值应触发告警
    m_alertManager.checkValue("testDevice", 0, 5.0);
    QCOMPARE(spy.count(), 1);
    
    Alert alert = spy.takeFirst().at(0).value<Alert>();
    QCOMPARE(alert.registerAddr, static_cast<quint16>(0));
    QCOMPARE(alert.value, 5.0);
    
    qDebug() << "低于最小值告警测试通过";
}

void TestAlertManager::testCheckValueAboveMax()
{
    AlertConfig config;
    config.registerAddr = 0;
    config.minValue = 0.0;
    config.maxValue = 50.0;
    config.enabled = true;
    
    m_alertManager.addAlertConfig("testDevice", config);
    
    QSignalSpy spy(&m_alertManager, &AlertManager::alertTriggered);
    
    // 超过最大值应触发告警
    m_alertManager.checkValue("testDevice", 0, 75.0);
    QCOMPARE(spy.count(), 1);
    
    qDebug() << "超过最大值告警测试通过";
}

void TestAlertManager::testCheckValueDisabled()
{
    AlertConfig config;
    config.registerAddr = 0;
    config.minValue = 0.0;
    config.maxValue = 50.0;
    config.enabled = false;  // 禁用
    
    m_alertManager.addAlertConfig("testDevice", config);
    
    QSignalSpy spy(&m_alertManager, &AlertManager::alertTriggered);
    
    // 禁用的配置不应触发告警
    m_alertManager.checkValue("testDevice", 0, 100.0);
    QCOMPARE(spy.count(), 0);
    
    qDebug() << "禁用配置告警测试通过";
}

void TestAlertManager::testRemoveAlertConfig()
{
    AlertConfig config;
    config.registerAddr = 0;
    config.minValue = 0.0;
    config.maxValue = 100.0;
    config.enabled = true;
    
    m_alertManager.addAlertConfig("testDevice", config);
    QCOMPARE(m_alertManager.allConfigsForDevice("testDevice").size(), 1);
    
    m_alertManager.removeAlertConfig("testDevice", 0);
    QCOMPARE(m_alertManager.allConfigsForDevice("testDevice").size(), 0);
    
    qDebug() << "告警配置删除测试通过";
}

void TestAlertManager::testClearDeviceConfigs()
{
    // 添加多个配置
    for (int i = 0; i < 5; ++i) {
        AlertConfig config;
        config.registerAddr = static_cast<quint16>(i);
        config.minValue = 0.0;
        config.maxValue = 100.0;
        config.enabled = true;
        m_alertManager.addAlertConfig("testDevice", config);
    }
    
    QCOMPARE(m_alertManager.allConfigsForDevice("testDevice").size(), 5);
    
    // 清空
    m_alertManager.clearDeviceConfigs("testDevice");
    QCOMPARE(m_alertManager.allConfigsForDevice("testDevice").size(), 0);
    
    qDebug() << "批量清空配置测试通过";
}

void TestAlertManager::testGetAlertHistory()
{
    AlertConfig config;
    config.registerAddr = 0;
    config.minValue = 0.0;
    config.maxValue = 50.0;
    config.enabled = true;
    
    m_alertManager.addAlertConfig("testDevice", config);
    
    // 触发一些告警
    m_alertManager.checkValue("testDevice", 0, 100.0);
    m_alertManager.checkValue("testDevice", 0, 200.0);
    
    // 查询历史
    QDateTime start = QDateTime::currentDateTime().addSecs(-3600);
    QDateTime end = QDateTime::currentDateTime();
    
    auto history = m_alertManager.getAlertHistory("testDevice", start, end);
    QVERIFY(history.size() >= 2);
    
    qDebug() << "告警历史查询测试通过，记录数:" << history.size();
}
