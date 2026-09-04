#ifndef TEST_RINGBUFFER_H
#define TEST_RINGBUFFER_H

#include <QObject>
#include <QtTest/QtTest>
#include "data/ringbuffer.h"

class TestRingBuffer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // RingBuffer 基础测试
    void testWriteAndRead();
    void testEmptyBuffer();
    void testFullBuffer();
    void testOverwriteMode();
    void testNoOverwriteMode();
    void testAvailableReadWrite();
    void testClear();
    void testCapacity();
};

#endif // TEST_RINGBUFFER_H
