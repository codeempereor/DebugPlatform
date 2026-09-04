#ifndef TEST_CRC16_H
#define TEST_CRC16_H

#include <QObject>
#include <QtTest/QtTest>

class TestCrc16 : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // CRC16 基础测试
    void testCrc16Empty();
    void testCrc16SingleByte();
    void testCrc16MultipleBytes();
    void testCrc16KnownValues();
    void testCrc16ModbusAlgorithm();
    
    // CRC16 边界测试
    void testCrc16LargeData();
    void testCrc16Consistency();
};

#endif // TEST_CRC16_H
