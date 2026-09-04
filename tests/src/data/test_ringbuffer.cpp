#include "test_ringbuffer.h"

void TestRingBuffer::initTestCase()
{
    qDebug() << "=== RingBuffer 测试套件开始 ===";
}

void TestRingBuffer::cleanupTestCase()
{
    qDebug() << "=== RingBuffer 测试套件结束 ===";
}

void TestRingBuffer::testWriteAndRead()
{
    RingBuffer<int> buffer(10);
    
    // 写入数据
    QVERIFY(buffer.write(1));
    QVERIFY(buffer.write(2));
    QVERIFY(buffer.write(3));
    
    // 读取数据
    int value;
    QVERIFY(buffer.read(value));
    QCOMPARE(value, 1);
    
    QVERIFY(buffer.read(value));
    QCOMPARE(value, 2);
    
    QVERIFY(buffer.read(value));
    QCOMPARE(value, 3);
    
    qDebug() << "RingBuffer 读写测试通过";
}

void TestRingBuffer::testEmptyBuffer()
{
    RingBuffer<int> buffer(10);
    
    QVERIFY(buffer.isEmpty());
    QCOMPARE(buffer.availableRead(), static_cast<size_t>(0));
    
    int value;
    QVERIFY(!buffer.read(value));
    
    qDebug() << "空缓冲区测试通过";
}

void TestRingBuffer::testFullBuffer()
{
    RingBuffer<int> buffer(3); // 容量为 3
    
    buffer.write(1);
    buffer.write(2);
    buffer.write(3);
    
    QVERIFY(buffer.isFull());
    QCOMPARE(buffer.availableWrite(), static_cast<size_t>(0));
    
    qDebug() << "满缓冲区测试通过";
}

void TestRingBuffer::testOverwriteMode()
{
    RingBuffer<int> buffer(3, true); // 覆盖模式
    
    // 填满缓冲区
    buffer.write(1);
    buffer.write(2);
    buffer.write(3);
    
    // 继续写入（应该覆盖最旧数据）
    QVERIFY(buffer.write(4));
    QVERIFY(buffer.write(5));
    
    // 读取应得到最新的数据
    int value;
    buffer.read(value);
    QCOMPARE(value, 3); // 被覆盖后的第一个有效值
    
    buffer.read(value);
    QCOMPARE(value, 4);
    
    buffer.read(value);
    QCOMPARE(value, 5);
    
    qDebug() << "覆盖模式测试通过";
}

void TestRingBuffer::testNoOverwriteMode()
{
    RingBuffer<int> buffer(3, false); // 不覆盖模式
    
    buffer.write(1);
    buffer.write(2);
    buffer.write(3);
    
    // 缓冲区已满，写入应失败
    QVERIFY(!buffer.write(4));
    
    qDebug() << "非覆盖模式测试通过";
}

void TestRingBuffer::testAvailableReadWrite()
{
    RingBuffer<int> buffer(10);
    
    QCOMPARE(buffer.availableWrite(), static_cast<size_t>(10));
    QCOMPARE(buffer.availableRead(), static_cast<size_t>(0));
    
    buffer.write(1);
    buffer.write(2);
    
    QCOMPARE(buffer.availableWrite(), static_cast<size_t>(8));
    QCOMPARE(buffer.availableRead(), static_cast<size_t>(2));
    
    qDebug() << "可用空间测试通过";
}

void TestRingBuffer::testClear()
{
    RingBuffer<int> buffer(10);
    
    buffer.write(1);
    buffer.write(2);
    buffer.write(3);
    
    buffer.clear();
    
    QVERIFY(buffer.isEmpty());
    QCOMPARE(buffer.availableRead(), static_cast<size_t>(0));
    QCOMPARE(buffer.availableWrite(), static_cast<size_t>(10));
    
    qDebug() << "清空测试通过";
}

void TestRingBuffer::testCapacity()
{
    RingBuffer<int> buffer(100);
    
    QCOMPARE(buffer.capacity(), static_cast<size_t>(100));
    
    qDebug() << "容量测试通过";
}
