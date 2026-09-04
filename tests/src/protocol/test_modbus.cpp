#include "test_modbus.h"
#include "protocol/modbusrtuprotocol.h"
#include "protocol/modbustcpprotocol.h"
#include <QSignalSpy>

void TestModbus::initTestCase()
{
    qDebug() << "=== Modbus 协议测试套件开始 ===";
}

void TestModbus::cleanupTestCase()
{
    qDebug() << "=== Modbus 协议测试套件结束 ===";
}

// ============ Modbus RTU 测试 ============

void TestModbus::testRtuReadHoldingRegisters()
{
    // 读保持寄存器 (0x03): 从站1，地址0，数量10
    QByteArray frame = ModbusRtuProtocol::buildReadHoldingRegisters(1, 0, 10);
    
    // 期望帧: 01 03 00 00 00 0A [CRC_L] [CRC_H]
    QCOMPARE(frame.size(), 8);
    QCOMPARE(static_cast<unsigned char>(frame[0]), 0x01); // 从站地址
    QCOMPARE(static_cast<unsigned char>(frame[1]), 0x03); // 功能码
    
    qDebug() << "RTU 读寄存器帧: " << frame.toHex(' ');
}

void TestModbus::testRtuWriteSingleRegister()
{
    // 写单寄存器 (0x06): 从站1，地址0，值100
    QByteArray frame = ModbusRtuProtocol::buildWriteSingleRegister(1, 0, 100);
    
    QCOMPARE(frame.size(), 8);
    QCOMPARE(static_cast<unsigned char>(frame[0]), 0x01); // 从站
    QCOMPARE(static_cast<unsigned char>(frame[1]), 0x06); // 功能码
    QCOMPARE(static_cast<unsigned char>(frame[2]), 0x00); // 地址高
    QCOMPARE(static_cast<unsigned char>(frame[3]), 0x00); // 地址低
    QCOMPARE(static_cast<unsigned char>(frame[4]), 0x00); // 值高
    QCOMPARE(static_cast<unsigned char>(frame[5]), 0x64); // 值低 (100)
    
    qDebug() << "RTU 写单寄存器帧: " << frame.toHex(' ');
}

void TestModbus::testRtuWriteMultipleRegisters()
{
    // 写多寄存器 (0x10): 从站1，起始地址0，数量3，值[100,200,300]
    QVector<quint16> values = {100, 200, 300};
    QByteArray frame = ModbusRtuProtocol::buildWriteMultipleRegisters(1, 0, values);
    
    // 帧结构: [地址][功能码][起始地址高][起始地址低][数量高][数量低][字节数]
    //         [值1高][值1低][值2高][值2低][值3高][值3低][CRC_L][CRC_H]
    // 1 + 1 + 2 + 2 + 1 + 6 + 2 = 15
    QCOMPARE(frame.size(), 15);
    
    QCOMPARE(static_cast<unsigned char>(frame[0]), 0x01); // 从站
    QCOMPARE(static_cast<unsigned char>(frame[1]), 0x10); // 功能码
    
    // 字节数应为 3*2 = 6
    QCOMPARE(static_cast<unsigned char>(frame[6]), 6);
    
    qDebug() << "RTU 写多寄存器帧: " << frame.toHex(' ');
}

void TestModbus::testRtuParseResponse()
{
    // 解析读寄存器响应
    // 响应: 从站1, 功能码03, 字节数04, 值100, 值200, CRC
    QByteArray response = QByteArray::fromHex("010304006400C8BA7A");
    
    ModbusResponse result = ModbusRtuProtocol::parseResponse(response);
    
    QVERIFY(!result.isException);
    QCOMPARE(result.slaveAddress, static_cast<quint8>(1));
    QCOMPARE(static_cast<quint8>(result.functionCode), static_cast<quint8>(0x03));
    QCOMPARE(result.data.size(), 2);
    QCOMPARE(result.data[0], static_cast<quint16>(100));  // 0x0064
    QCOMPARE(result.data[1], static_cast<quint16>(200));  // 0x00C8
    
    qDebug() << "RTU 响应解析: 值 =" << result.data;
}

void TestModbus::testRtuParseException()
{
    // 解析异常响应: 从站1，非法功能码异常
    QByteArray exception = QByteArray::fromHex("018302C0F1");
    
    ModbusResponse result = ModbusRtuProtocol::parseResponse(exception);
    
    QVERIFY(result.isException);
    QCOMPARE(result.slaveAddress, static_cast<quint8>(1));
    QCOMPARE(static_cast<quint8>(result.functionCode), static_cast<quint8>(0x83)); // 异常功能码
    QCOMPARE(result.exceptionCode, ModbusExceptionCode::IllegalDataAddress); // 0x02
    
    qDebug() << "RTU 异常码: " << ModbusRtuProtocol::exceptionCodeToString(result.exceptionCode);
}

// ============ Modbus TCP 测试 ============

void TestModbus::testTcpBuildReadRequest()
{
    ModbusTcpProtocol tcpProto(1);
    
    // 构建读寄存器请求
    QByteArray frame = tcpProto.buildReadHoldingRegisters(1, 0, 10);
    
    // 帧结构: [MBAP头7字节][PDU]
    // MBAP: [TID_H][TID_L][PID_H][PID_L][Length_H][Length_L][UnitID]
    QVERIFY(frame.size() >= 7);
    
    // 验证 MBAP 头
    quint16 pid = (static_cast<unsigned char>(frame[2]) << 8) | 
                   static_cast<unsigned char>(frame[3]);
    quint8 unitId = static_cast<unsigned char>(frame[6]);
    
    QCOMPARE(pid, static_cast<quint16>(0)); // 协议ID
    QCOMPARE(unitId, static_cast<quint8>(1)); // Unit ID
    
    qDebug() << "TCP 读请求帧: " << frame.toHex(' ');
}

void TestModbus::testTcpBuildWriteSingleRequest()
{
    ModbusTcpProtocol tcpProto(1);
    
    QByteArray frame = tcpProto.buildWriteSingleRegister(1, 0, 100);
    
    QVERIFY(frame.size() >= 7);
    
    // PDU 功能码应为 0x06
    quint8 functionCode = static_cast<unsigned char>(frame[7]);
    QCOMPARE(functionCode, static_cast<quint8>(0x06));
    
    qDebug() << "TCP 写单寄存器帧: " << frame.toHex(' ');
}

void TestModbus::testTcpBuildWriteMultipleRequest()
{
    ModbusTcpProtocol tcpProto(1);
    QVector<quint16> values = {100, 200, 300};
    
    QByteArray frame = tcpProto.buildWriteMultipleRegisters(1, 0, values);
    
    QVERIFY(frame.size() >= 7);
    
    // PDU 功能码应为 0x10
    quint8 functionCode = static_cast<unsigned char>(frame[7]);
    QCOMPARE(functionCode, static_cast<quint8>(0x10));
    
    qDebug() << "TCP 写多寄存器帧: " << frame.toHex(' ');
}

void TestModbus::testTcpParseReadResponse()
{
    // 模拟 Modbus TCP 读响应
    // MBAP: TID=1, PID=0, Length=9(UnitID+PDU), UnitID=1
    // PDU: [03][04][0064][00C8]
    QByteArray response = QByteArray::fromHex(
        "000100000007010304006400C8"
    );
    
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(response);
    
    QVERIFY(!result.isException);
    QCOMPARE(result.data.size(), 2);
    QCOMPARE(result.data[0], static_cast<quint16>(100));
    QCOMPARE(result.data[1], static_cast<quint16>(200));
    
    qDebug() << "TCP 响应解析通过";
}

void TestModbus::testTcpParseExceptionResponse()
{
    // Modbus TCP 异常响应
    QByteArray exception = QByteArray::fromHex(
        "000100000003018302"  // MBAP + PDU[异常功能码+异常码]
    );
    
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(exception);
    
    QVERIFY(result.isException);
    QCOMPARE(result.exceptionCode, ModbusExceptionCode::IllegalDataAddress);
    
    qDebug() << "TCP 异常响应解析通过";
}

// ============ 对比测试 ============

void TestModbus::testRtuVsTcpFrameSize()
{
    // 相同功能的 RTU 和 TCP 帧大小对比
    QByteArray rtuFrame = ModbusRtuProtocol::buildReadHoldingRegisters(1, 0, 10);
    
    ModbusTcpProtocol tcpProto(1);
    QByteArray tcpFrame = tcpProto.buildReadHoldingRegisters(1, 0, 10);
    
    // TCP 帧比 RTU 帧大（MBAP 头 7 字节 vs RTU CRC 2 字节）
    QVERIFY(tcpFrame.size() > rtuFrame.size());
    
    qDebug() << "RTU 帧大小:" << rtuFrame.size() 
             << "vs TCP 帧大小:" << tcpFrame.size();
}

void TestModbus::testProtocolConsistency()
{
    // 验证相同的寄存器值在两种协议中一致
    quint16 testValue = 12345;
    
    QByteArray rtuFrame = ModbusRtuProtocol::buildWriteSingleRegister(1, 0, testValue);
    
    ModbusTcpProtocol tcpProto(1);
    QByteArray tcpFrame = tcpProto.buildWriteSingleRegister(1, 0, testValue);
    
    // 提取 RTU 帧中的值
    quint16 rtuValue = (static_cast<unsigned char>(rtuFrame[4]) << 8) |
                        static_cast<unsigned char>(rtuFrame[5]);
    
    // 提取 TCP 帧中的值（PDU 偏移 3+2 = 5）
    // MBAP(7) + PDU: [func(1)][addr(2)][value(2)]
    quint16 tcpValue = (static_cast<unsigned char>(tcpFrame[10]) << 8) |
                        static_cast<unsigned char>(tcpFrame[11]);
    
    QCOMPARE(rtuValue, testValue);
    QCOMPARE(tcpValue, testValue);
    
    qDebug() << "协议一致性验证通过";
}


// ============================================================================
// P1-3a 错误路径测试 (Modbus TCP)
// ============================================================================

void TestModbus::testTcpParseResponseTooShort()
{
    // 响应小于 8 字节（MBAP 7B + 功能码 1B），应返回异常
    QByteArray shortResp = QByteArray::fromHex("00010000000001");
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(shortResp);
    QVERIFY(result.isException);
    QCOMPARE(result.exceptionCode, ModbusExceptionCode::SlaveDeviceFailure);

    qDebug() << "过短 TCP 响应验证通过";
}

void TestModbus::testTcpParseInvalidProtocolId()
{
    // 协议 ID != 0 (PROTOCOL_ID)，应返回异常
    // MBAP: TID=0001, ProtocolID=0001 (非法), Length=0001, UnitID=01
    // PDU: 0x03 (功能码)
    QByteArray badResp = QByteArray::fromHex("0001000100010103");
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(badResp);
    QVERIFY(result.isException);
    QCOMPARE(result.exceptionCode, ModbusExceptionCode::SlaveDeviceFailure);

    qDebug() << "非法协议 ID 验证通过";
}

void TestModbus::testTcpParseInvalidLength()
{
    // MBAP length = 0 (非法，应 [1, 254])，应返回异常
    QByteArray badResp = QByteArray::fromHex("0001000000000103");
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(badResp);
    QVERIFY(result.isException);
    QCOMPARE(result.exceptionCode, ModbusExceptionCode::SlaveDeviceFailure);

    // 测试 length > 254
    badResp = QByteArray::fromHex("00010000FF010103");
    result = ModbusTcpProtocol::parseResponseRaw(badResp);
    QVERIFY(result.isException);

    qDebug() << "非法 length 字段验证通过";
}

void TestModbus::testTcpParseTransactionMismatch()
{
    // 构造合法响应，但 TID 与期望值不匹配
    QByteArray resp = QByteArray::fromHex("000100000003010304006400C8XXXX");
    // 使用更简单的合法响应：TID=0001, ProtoID=0, Len=4, Unit=1, Func=03, byteCount=2, val1=0x0064
    resp = QByteArray::fromHex("0001000000040103020064");
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(resp);
    QVERIFY(!result.isException);  // tid=0 时跳过检查

    // 指定不匹配的 transactionId 应报异常
    ModbusTcpProtocol parser;
    ModbusResponse result2 = parser.parseResponse(resp, 0x0002);  // 期望 0002 但响应是 0001
    QVERIFY(result2.isException);
    QCOMPARE(result2.exceptionCode, ModbusExceptionCode::SlaveDeviceFailure);

    qDebug() << "事务 ID 不匹配验证通过";
}

void TestModbus::testTcpParseExceptionNoCode()
{
    // 异常响应：功能码 | 0x80，但没有 exception code 字节（被截断）
    // MBAP: TID=0001, ProtoID=0, Len=0002, Unit=01, PDU: 0x83 (异常)
    QByteArray truncatedExc = QByteArray::fromHex("0001000000020183");
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(truncatedExc);
    QVERIFY(result.isException);
    // exception code 字节缺失，应回退到 SlaveDeviceFailure
    QCOMPARE(result.exceptionCode, ModbusExceptionCode::SlaveDeviceFailure);

    qDebug() << "异常码缺失验证通过";
}

void TestModbus::testTcpParseUnknownFunctionCode()
{
    // 未知功能码 0x55（非 Modbus 标准功能码）
    // MBAP: TID=0001, ProtoID=0, Len=0002, Unit=01, PDU: 0x55 + 一个字节
    QByteArray resp = QByteArray::fromHex("000100000002015500");
    ModbusResponse result = ModbusTcpProtocol::parseResponseRaw(resp);
    QVERIFY(result.isException);
    QCOMPARE(result.exceptionCode, ModbusExceptionCode::IllegalFunction);

    qDebug() << "未知功能码验证通过";
}
