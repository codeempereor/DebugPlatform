#include "test_datapersistent.h"
#include <new>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QVariantMap>

void TestDataPersistent::initTestCase()
{
    qDebug() << "=== DataPersistent 测试套件开始 ===";
    
    // 使用临时数据库路径
    m_testDbPath = QDir::tempPath() + "/test_debug_platform.db";
    
    // 清理可能存在的旧测试数据库
    QFile::remove(m_testDbPath);
}

void TestDataPersistent::cleanupTestCase()
{
    qDebug() << "=== DataPersistent 测试套件结束 ===";
    
    // 清理测试数据库
    QFile::remove(m_testDbPath);
}

void TestDataPersistent::init()
{
    // 每个测试前删除旧数据库
    QFile::remove(m_testDbPath);
    
    // 创建新的 DataPersistent 实例
    m_db = new DataPersistent(this);
    QVERIFY(m_db->initialize(m_testDbPath));
    QVERIFY(m_db->isReady());
    
    // 先添加测试设备，避免外键约束失败
    m_db->addDevice("test_device_1", "Test Device 1", "modbus", "{}");
    m_db->addDevice("device_A", "Device A", "modbus", "{}");
    m_db->addDevice("device_B", "Device B", "modbus", "{}");
    m_db->addDevice("timerange_test", "Timerange Test", "modbus", "{}");
    m_db->addDevice("cleanup_test", "Cleanup Test", "modbus", "{}");
    m_db->addDevice("alert_device", "Alert Device", "modbus", "{}");
    m_db->addDevice("batch_device", "Batch Device", "modbus", "{}");
}

void TestDataPersistent::cleanup()
{
    // 每个测试后清理
    if (m_db) {
        delete m_db;
        m_db = nullptr;
    }
}

void TestDataPersistent::testInitialize()
{
    QVERIFY(m_db->isReady());
    qDebug() << "数据库初始化测试通过";
}

void TestDataPersistent::testInsertRegisterData()
{
    bool ok = m_db->insertRegisterData("test_device_1", 0, 123.456);
    QVERIFY(ok);
    
    // 测试多个寄存器地址
    QVERIFY(m_db->insertRegisterData("test_device_1", 1, 0.0));
    QVERIFY(m_db->insertRegisterData("test_device_1", 65535, 9999.999));
    
    qDebug() << "数据点保存测试通过";
}

void TestDataPersistent::testQueryRegisterData()
{
    for (int i = 0; i < 10; ++i) {
        m_db->insertRegisterData("test_device_1", 
            static_cast<quint16>(i), 
            static_cast<double>(i * 10));
    }
    
    auto results = m_db->queryRegisterData("test_device_1", 0, 
        QDateTime().addYears(-1), QDateTime::currentDateTimeUtc());
    
    QVERIFY(results.size() >= 1);
    
    // 验证返回的数据内容
    if (results.size() > 0) {
        QVariantMap first = results.first();
        QCOMPARE(first["device_id"].toString(), QString("test_device_1"));
        QCOMPARE(first["register_addr"].toInt(), 0);
    }
    
    qDebug() << "数据查询测试通过，返回" << results.size() << "条记录";
}

void TestDataPersistent::testQueryByDeviceId()
{
    for (int i = 0; i < 5; ++i) {
        m_db->insertRegisterData("device_A", 0, static_cast<double>(i));
    }
    
    for (int i = 0; i < 3; ++i) {
        m_db->insertRegisterData("device_B", 0, static_cast<double>(i + 100));
    }
    
    auto resultsA = m_db->queryRegisterData("device_A", 0,
        QDateTime().addYears(-1), QDateTime::currentDateTimeUtc());
    auto resultsB = m_db->queryRegisterData("device_B", 0,
        QDateTime().addYears(-1), QDateTime::currentDateTimeUtc());
    
    QVERIFY(resultsA.size() >= 5);
    QVERIFY(resultsB.size() >= 3);
    
    qDebug() << "按设备ID查询测试通过";
}

void TestDataPersistent::testQueryByTimeRange()
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime oneHourAgo = now.addSecs(-3600);
    QDateTime twoHoursAgo = now.addSecs(-7200);
    
    for (int i = 0; i < 5; ++i) {
        m_db->insertRegisterData("timerange_test", 0, static_cast<double>(i));
    }
    
    auto results = m_db->queryRegisterData("timerange_test", 0,
        oneHourAgo, now);
    QVERIFY(results.size() >= 5);
    
    auto oldResults = m_db->queryRegisterData("timerange_test", 0,
        twoHoursAgo, oneHourAgo);
    QCOMPARE(oldResults.size(), 0);
    
    qDebug() << "时间范围查询测试通过";
}

void TestDataPersistent::testCleanupOldData()
{
    for (int i = 0; i < 10; ++i) {
        m_db->insertRegisterData("cleanup_test", 
            static_cast<quint16>(i), 
            static_cast<double>(i));
    }
    
    auto results = m_db->queryRegisterData("cleanup_test", 0,
        QDateTime::currentDateTimeUtc().addDays(-1), QDateTime::currentDateTimeUtc());
    QVERIFY(results.size() >= 1);
    
    m_db->cleanupOldData(30);
    
    auto resultsAfter = m_db->queryRegisterData("cleanup_test", 0,
        QDateTime::currentDateTimeUtc().addDays(-1), QDateTime::currentDateTimeUtc());
    QVERIFY(resultsAfter.size() >= 1);
    
    qDebug() << "过期数据清理测试通过";
}

void TestDataPersistent::testConfigSaveLoad()
{
    bool saveOk = m_db->saveConfig("test_key", "test_value");
    QVERIFY(saveOk);
    
    QString value = m_db->loadConfig("test_key");
    QCOMPARE(value, QString("test_value"));
    
    m_db->saveConfig("test_key2", "value2");
    QString value2 = m_db->loadConfig("test_key2");
    QCOMPARE(value2, QString("value2"));
    
    // 测试配置更新（ON CONFLICT DO UPDATE）
    QVERIFY(m_db->saveConfig("test_key", "updated_value"));
    QString updated = m_db->loadConfig("test_key");
    QCOMPARE(updated, QString("updated_value"));
    
    qDebug() << "配置保存/加载测试通过";
}

// ========== 设备管理测试 ==========

void TestDataPersistent::testAddDevice()
{
    bool ok = m_db->addDevice("new_device", "New Device", "modbus_tcp", "{\"port\":502}");
    QVERIFY(ok);
    
    // 验证设备已添加
    auto devices = m_db->loadAllDevices();
    bool found = false;
    for (const auto &dev : devices) {
        if (dev["id"].toString() == "new_device") {
            found = true;
            QCOMPARE(dev["name"].toString(), QString("New Device"));
            QCOMPARE(dev["type"].toString(), QString("modbus_tcp"));
            break;
        }
    }
    QVERIFY(found);
    
    qDebug() << "添加设备测试通过";
}

void TestDataPersistent::testAddDeviceConflict()
{
    // 第一次添加
    QVERIFY(m_db->addDevice("conflict_device", "Original Name", "modbus", "{}"));
    
    // 第二次添加相同ID（应更新而非报错）
    QVERIFY(m_db->addDevice("conflict_device", "Updated Name", "serial", "{\"baud\":9600}"));
    
    // 验证已更新
    auto devices = m_db->loadAllDevices();
    for (const auto &dev : devices) {
        if (dev["id"].toString() == "conflict_device") {
            QCOMPARE(dev["name"].toString(), QString("Updated Name"));
            QCOMPARE(dev["type"].toString(), QString("serial"));
            break;
        }
    }
    
    qDebug() << "设备冲突更新测试通过";
}

void TestDataPersistent::testRemoveDevice()
{
    // 先添加一个可以删除的设备
    QVERIFY(m_db->addDevice("to_remove", "To Remove", "modbus", "{}"));
    
    // 添加关联数据（测试级联删除）
    QVERIFY(m_db->insertRegisterData("to_remove", 0, 42.0));
    QVERIFY(m_db->insertAlert("to_remove", 0, 42.0, "WARNING", "Test alert"));
    
    // 删除设备
    bool ok = m_db->removeDevice("to_remove");
    QVERIFY(ok);
    
    // 验证设备已删除
    auto devices = m_db->loadAllDevices();
    for (const auto &dev : devices) {
        QVERIFY(dev["id"].toString() != "to_remove");
    }
    
    qDebug() << "删除设备测试通过";
}

void TestDataPersistent::testLoadAllDevices()
{
    auto devices = m_db->loadAllDevices();
    
    // 验证至少有初始设备
    QVERIFY(devices.size() >= 5);
    
    // 验证每个设备都有必要字段
    for (const auto &dev : devices) {
        QVERIFY(dev.contains("id"));
        QVERIFY(dev.contains("name"));
        QVERIFY(dev.contains("type"));
        QVERIFY(dev.contains("config"));
    }
    
    qDebug() << "加载所有设备测试通过，共" << devices.size() << "个设备";
}

// ========== 批量数据测试 ==========

void TestDataPersistent::testInsertRegisterDataBatch()
{
    QVector<QPair<QString, QPair<quint16, double>>> batch;
    
    for (int i = 0; i < 100; ++i) {
        QPair<QString, QPair<quint16, double>> item;
        item.first = "batch_device";
        item.second.first = static_cast<quint16>(i);
        item.second.second = static_cast<double>(i * 1.5);
        batch.append(item);
    }
    
    bool ok = m_db->insertRegisterDataBatch(batch);
    QVERIFY(ok);
    
    // 验证数据已写入
    auto results = m_db->queryRegisterData("batch_device", 0,
        QDateTime().addYears(-1), QDateTime::currentDateTimeUtc());
    QVERIFY(results.size() >= 1);
    
    qDebug() << "批量插入数据测试通过";
}

void TestDataPersistent::testInsertRegisterDataBatchEmpty()
{
    QVector<QPair<QString, QPair<quint16, double>>> emptyBatch;
    
    bool ok = m_db->insertRegisterDataBatch(emptyBatch);
    QVERIFY(!ok);  // 空批次应返回false
    
    qDebug() << "空批量数据测试通过";
}

// ========== 告警测试 ==========

void TestDataPersistent::testInsertAlert()
{
    bool ok = m_db->insertAlert("alert_device", 100, 25.5, "WARNING", "温度过高");
    QVERIFY(ok);
    
    ok = m_db->insertAlert("alert_device", 200, 3.14, "CRITICAL", "压力异常");
    QVERIFY(ok);
    
    qDebug() << "告警插入测试通过";
}

void TestDataPersistent::testQueryAlerts()
{
    // 先插入一些告警
    m_db->insertAlert("alert_device", 100, 25.5, "WARNING", "温度过高");
    m_db->insertAlert("alert_device", 200, 3.14, "CRITICAL", "压力异常");
    m_db->insertAlert("alert_device", 300, 100.0, "INFO", "状态更新");
    
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime dayAgo = now.addDays(-1);
    
    auto results = m_db->queryAlerts("alert_device", dayAgo, now);
    
    QVERIFY(results.size() >= 3);
    
    // 验证告警记录内容
    if (results.size() > 0) {
        QVariantMap first = results.first();
        QVERIFY(first.contains("device_id"));
        QVERIFY(first.contains("level"));
        QVERIFY(first.contains("message"));
    }
    
    qDebug() << "告警查询测试通过，共" << results.size() << "条记录";
}

void TestDataPersistent::testQueryRecentAlerts()
{
    // 插入多个告警
    for (int i = 0; i < 15; ++i) {
        m_db->insertAlert("alert_device", 
            static_cast<quint16>(i), 
            static_cast<double>(i * 10),
            "WARNING", 
            QString("Alert %1").arg(i));
    }
    
    // 查询最近5条
    auto recent5 = m_db->queryRecentAlerts(5);
    QCOMPARE(recent5.size(), 5);
    
    // 查询最近10条
    auto recent10 = m_db->queryRecentAlerts(10);
    QCOMPARE(recent10.size(), 10);
    
    // 查询最近0条（应返回空）
    auto recent0 = m_db->queryRecentAlerts(0);
    QCOMPARE(recent0.size(), 0);
    
    // 查询负数（应返回空）
    auto recentNeg = m_db->queryRecentAlerts(-1);
    QCOMPARE(recentNeg.size(), 0);
    
    qDebug() << "最近告警查询测试通过";
}

// ========== 错误处理测试 ==========

void TestDataPersistent::testNotInitialized()
{
    // 创建一个未初始化的实例
    DataPersistent uninitialized;
    
    // 所有操作应返回false或空
    QVERIFY(!uninitialized.addDevice("id", "name", "type", "config"));
    QVERIFY(!uninitialized.removeDevice("id"));
    QVERIFY(!uninitialized.insertRegisterData("dev", 0, 0.0));
    QVERIFY(!uninitialized.insertAlert("dev", 0, 0.0, "level", "msg"));
    QVERIFY(!uninitialized.saveConfig("key", "value"));
    QCOMPARE(uninitialized.loadConfig("key"), QString());
    QVERIFY(uninitialized.loadAllDevices().isEmpty());
    QVERIFY(uninitialized.queryRegisterData("dev", 0, QDateTime(), QDateTime()).isEmpty());
    QVERIFY(uninitialized.queryAlerts("dev", QDateTime(), QDateTime()).isEmpty());
    QVERIFY(uninitialized.queryRecentAlerts(10).isEmpty());
    QVERIFY(!uninitialized.isReady());
    
    qDebug() << "未初始化错误处理测试通过";
}

void TestDataPersistent::testDoubleInitialize()
{
    // 再次初始化（使用相同路径）应成功
    bool ok = m_db->initialize(m_testDbPath);
    QVERIFY(ok);
    QVERIFY(m_db->isReady());
    
    // 使用不同路径重新初始化
    QString newPath = QDir::tempPath() + "/test_debug_platform2.db";
    ok = m_db->initialize(newPath);
    QVERIFY(ok);
    QVERIFY(m_db->isReady());
    
    // 清理第二个数据库
    QFile::remove(newPath);
    
    qDebug() << "重复初始化测试通过";
}

void TestDataPersistent::testCleanupEdgeCases()
{
    // 先插入一些测试数据
    for (int i = 0; i < 5; ++i) {
        m_db->insertRegisterData("cleanup_test",
            static_cast<quint16>(i),
            static_cast<double>(i));
    }
    
    // daysRetention = 0 不应执行删除
    m_db->cleanupOldData(0);
    
    // daysRetention 负数不应执行删除
    m_db->cleanupOldData(-5);
    
    // 验证数据仍完好
    auto results = m_db->queryRegisterData("cleanup_test", 0,
        QDateTime::currentDateTimeUtc().addDays(-1), QDateTime::currentDateTimeUtc());
    QVERIFY(results.size() >= 1);
    
    qDebug() << "清理边界条件测试通过";
}

void TestDataPersistent::testConfigNonexistent()
{
    // 查询不存在的配置应返回空字符串
    QString value = m_db->loadConfig("nonexistent_key_12345");
    QCOMPARE(value, QString());
    
    // 查询空key配置
    QString value2 = m_db->loadConfig("");
    QCOMPARE(value2, QString());
    
    qDebug() << "不存在配置查询测试通过";
}
// ========== P1-3a 错误路径测试 ==========

void TestDataPersistent::testDatabaseClosed()
{
    // 先验证数据库就绪
    QVERIFY(m_db->isReady());
    QVERIFY(m_db->insertRegisterData("test_device_1", 0, 1.0));

    // 关闭数据库连接，模拟 SQLite 断连
    m_db->database().close();
    QVERIFY(!m_db->isReady());

    // 所有写操作应返回 false
    QVERIFY(!m_db->insertRegisterData("test_device_1", 0, 2.0));
    QVERIFY(!m_db->insertAlert("test_device_1", 0, 0.0, "WARNING", "msg"));
    QVERIFY(!m_db->saveConfig("key", "value"));
    QVERIFY(!m_db->addDevice("x", "y", "z", "{}"));
    QVERIFY(!m_db->removeDevice("test_device_1"));

    // 所有读操作应返回空
    QVERIFY(m_db->loadAllDevices().isEmpty());
    QVERIFY(m_db->queryRegisterData("test_device_1", 0,
        QDateTime::currentDateTimeUtc().addDays(-1),
        QDateTime::currentDateTimeUtc()).isEmpty());
    QVERIFY(m_db->queryAlerts("test_device_1",
        QDateTime::currentDateTimeUtc().addDays(-1),
        QDateTime::currentDateTimeUtc()).isEmpty());
    QVERIFY(m_db->queryRecentAlerts(10).isEmpty());
    QCOMPARE(m_db->loadConfig("test_key"), QString());

    qDebug() << "数据库关闭错误处理测试通过";
}

void TestDataPersistent::testForeignKeyViolation()
{
    // 外键约束已开启（PRAGMA foreign_keys=ON）
    // 插入寄存器数据使用不存在的设备ID，应失败
    QVERIFY(!m_db->insertRegisterData("nonexistent_device", 0, 1.0));

    // 插入告警使用不存在的设备ID，应失败
    QVERIFY(!m_db->insertAlert("nonexistent_device", 0, 1.0, "WARNING", "test"));

    // 批量插入包含不存在设备ID，事务应回滚
    QVector<QPair<QString, QPair<quint16, double>>> batch;
    QPair<QString, QPair<quint16, double>> item;
    item.first = "nonexistent_device";
    item.second.first = 0;
    item.second.second = 1.0;
    batch.append(item);
    QVERIFY(!m_db->insertRegisterDataBatch(batch));

    // 验证确实没有数据被写入
    QVERIFY(m_db->queryRegisterData("nonexistent_device", 0,
        QDateTime::currentDateTimeUtc().addDays(-1),
        QDateTime::currentDateTimeUtc()).isEmpty());

    qDebug() << "外键约束错误处理测试通过";
}

void TestDataPersistent::testQueryNoResults()
{
    // 先插入一些数据到已有设备
    QVERIFY(m_db->insertRegisterData("test_device_1", 0, 42.0));

    // 查询不存在的设备，应返回空
    QVERIFY(m_db->queryRegisterData("nonexistent_device", 0,
        QDateTime::currentDateTimeUtc().addDays(-1),
        QDateTime::currentDateTimeUtc()).isEmpty());

    // 查询存在设备但不存在的寄存器地址，应返回空
    QVERIFY(m_db->queryRegisterData("test_device_1", 999,
        QDateTime::currentDateTimeUtc().addDays(-1),
        QDateTime::currentDateTimeUtc()).isEmpty());

    // 时间范围倒置（start > end），应返回空
    QDateTime now = QDateTime::currentDateTimeUtc();
    QVERIFY(m_db->queryRegisterData("test_device_1", 0,
        now, now.addDays(-1)).isEmpty());

    // 查询不存在的告警
    QVERIFY(m_db->queryAlerts("nonexistent_device",
        QDateTime::currentDateTimeUtc().addDays(-1),
        QDateTime::currentDateTimeUtc()).isEmpty());

    qDebug() << "空查询结果测试通过";
}

