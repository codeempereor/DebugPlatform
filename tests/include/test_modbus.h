#ifndef TEST_MODBUS_H
#define TEST_MODBUS_H

#include <QObject>
#include <QtTest/QtTest>

class TestModbus : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // Modbus RTU 协议测试
    void testRtuReadHoldingRegisters();
    void testRtuWriteSingleRegister();
    void testRtuWriteMultipleRegisters();
    void testRtuParseResponse();
    void testRtuParseException();
    
    // Modbus TCP 协议测试
    void testTcpBuildReadRequest();
    void testTcpBuildWriteSingleRequest();
    void testTcpBuildWriteMultipleRequest();
    void testTcpParseReadResponse();
    void testTcpParseExceptionResponse();
    
    // 协议对比测试
    void testRtuVsTcpFrameSize();
    void testProtocolConsistency();

    // P1-3a 错误路径测试 (Modbus TCP)
    void testTcpParseResponseTooShort();
    void testTcpParseInvalidProtocolId();
    void testTcpParseInvalidLength();
    void testTcpParseTransactionMismatch();
    void testTcpParseExceptionNoCode();
    void testTcpParseUnknownFunctionCode();
};

#endif // TEST_MODBUS_H
