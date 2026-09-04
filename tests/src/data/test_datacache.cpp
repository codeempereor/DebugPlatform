#include "test_datacache.h"
#include "data/datacache.h"

void TestDataCache::initTestCase()
{
    qDebug() << "=== DataCache 测试套件开始 ===";
}

void TestDataCache::cleanupTestCase()
{
    qDebug() << "=== DataCache 测试套件结束 ===";
}

void TestDataCache::init()
{
    // 每个测试前的初始化
}

void TestDataCache::cleanup()
{
    // 每个测试后的清理
}

void TestDataCache::testUpdateAndGetValue()
{
    DataCache cache;
    
    // 初始值应为 0
    QCOMPARE(cache.getValue("device1", 0), 0.0);
    
    // 更新值
    cache.updateValue("device1", 0, 123.456);
    QCOMPARE(cache.getValue("device1", 0), 123.456);
    
    qDebug() << "DataCache 读写测试通过";
}

void TestDataCache::testMultipleDevices()
{
    DataCache cache;
    
    // 为不同设备设置值
    cache.updateValue("device1", 0, 100.0);
    cache.updateValue("device2", 0, 200.0);
    
    QCOMPARE(cache.getValue("device1", 0), 100.0);
    QCOMPARE(cache.getValue("device2", 0), 200.0);
    
    // 修改一个设备不影响另一个
    cache.updateValue("device1", 0, 150.0);
    QCOMPARE(cache.getValue("device1", 0), 150.0);
    QCOMPARE(cache.getValue("device2", 0), 200.0);
    
    qDebug() << "多设备缓存测试通过";
}

void TestDataCache::testValueChangedSignal()
{
    DataCache cache;
    
    QSignalSpy spy(&cache, &DataCache::valueChanged);
    QVERIFY(spy.isValid());
    
    cache.updateValue("device1", 0, 100.0);
    
    // 信号应该发射一次
    QCOMPARE(spy.count(), 1);
    
    // 获取信号参数
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("device1"));
    QCOMPARE(args.at(1).value<quint16>(), static_cast<quint16>(0));
    QCOMPARE(args.at(2).value<double>(), 100.0);
    
    qDebug() << "valueChanged 信号测试通过";
}

void TestDataCache::testDeviceDataCleared()
{
    DataCache cache;
    
    // 先设置一些值
    cache.updateValue("device1", 0, 100.0);
    cache.updateValue("device1", 1, 200.0);
    
    // 清空设备数据
    cache.clearDeviceData("device1");
    
    // 验证已清空
    QCOMPARE(cache.getValue("device1", 0), 0.0);
    QCOMPARE(cache.getValue("device1", 1), 0.0);
    
    qDebug() << "设备数据清空测试通过";
}

void TestDataCache::testGetDeviceList()
{
    DataCache cache;
    
    // 初始设备列表
    QStringList initialList = cache.getDeviceList();
    int initialCount = initialList.size();
    
    // 添加数据后设备应出现在列表
    cache.updateValue("newDevice", 0, 100.0);
    
    QStringList updatedList = cache.getDeviceList();
    QVERIFY(updatedList.contains("newDevice"));
    
    qDebug() << "设备列表测试通过";
}

void TestDataCache::testClearDeviceData()
{
    DataCache cache;
    
    // 设置多寄存器
    cache.updateValue("device1", 0, 100.0);
    cache.updateValue("device1", 1, 200.0);
    cache.updateValue("device1", 2, 300.0);
    
    // 清空
    cache.clearDeviceData("device1");
    
    // 验证全部为 0
    QCOMPARE(cache.getValue("device1", 0), 0.0);
    QCOMPARE(cache.getValue("device1", 1), 0.0);
    QCOMPARE(cache.getValue("device1", 2), 0.0);
    
    qDebug() << "批量清空测试通过";
}