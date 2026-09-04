#include "test_crc16.h"
#include "protocol/crc16.h"

void TestCrc16::initTestCase()
{
    qDebug() << "=== CRC16 测试套件开始 ===";
}

void TestCrc16::cleanupTestCase()
{
    qDebug() << "=== CRC16 测试套件结束 ===";
}

void TestCrc16::testCrc16Empty()
{
    // 空数据的 CRC 应该为初始值 0xFFFF
    QByteArray empty;
    quint16 crc = Crc16::calculate(empty);
    QCOMPARE(crc, static_cast<quint16>(0xFFFF));
    qDebug() << "空数据 CRC: " << Qt::hex << crc;
}

void TestCrc16::testCrc16SingleByte()
{
    // 单字节数据 0x01
    QByteArray data = QByteArray::fromHex("01");
    quint16 crc = Crc16::calculate(data);
    
    // CRC16(0x01) = 0x807E
    QCOMPARE(crc, static_cast<quint16>(0x807E));
    qDebug() << "单字节 CRC (0x01): " << Qt::hex << crc;
}

void TestCrc16::testCrc16MultipleBytes()
{
    // 多字节数据
    QByteArray data = QByteArray::fromHex("01030000000A");
    quint16 crc = Crc16::calculate(data);
    
    // Modbus 读 10 个寄存器的请求 CRC
    // CRC16([0x01, 0x03, 0x00, 0x00, 0x00, 0x0A]) = 0xCDC5
    QCOMPARE(crc, static_cast<quint16>(0xCDC5));
    qDebug() << "Modbus 请求 CRC: " << Qt::hex << crc;
}

void TestCrc16::testCrc16KnownValues()
{
    // 已知测试向量
    struct TestVector {
        QByteArray data;
        quint16 expectedCrc;
        const char* description;
    };
    
    TestVector vectors[] = {
        // Modbus 读2个寄存器请求
        { QByteArray::fromHex("010300000002"), 0x0BC4, "Modbus 读2个寄存器请求" },
        // Modbus 读响应
        { QByteArray::fromHex("0103040000000A0000"), 0xB722, "Modbus 读响应" },
        // Modbus 写单寄存器
        { QByteArray::fromHex("01060000000D"), 0x0F48, "Modbus 写单寄存器" },
    };
    
    for (const auto& vec : vectors) {
        quint16 crc = Crc16::calculate(vec.data);
        QCOMPARE(crc, vec.expectedCrc);
        qDebug() << vec.description << ": CRC=" << Qt::hex << crc;
    }
}

void TestCrc16::testCrc16ModbusAlgorithm()
{
    // 验证 Modbus RTU 的 CRC 计算
    // 从站地址1，功能码03，起始地址0，数量10
    QByteArray request = QByteArray::fromHex("01030000000A");
    quint16 crc = Crc16::calculate(request);
    
    qDebug() << "Modbus RTU 请求帧 CRC:" << Qt::hex << crc;
    // CRC16 = 0xCDC5 (寄存器值)
    QCOMPARE(crc, static_cast<quint16>(0xCDC5));
}

void TestCrc16::testCrc16LargeData()
{
    // 较大数据量测试
    QByteArray largeData(1000, static_cast<char>(0xAA));
    quint16 crc = Crc16::calculate(largeData);
    
    // 验证能正确计算（不崩溃）
    QVERIFY(crc != 0xFFFF);
    qDebug() << "大数据量 CRC (1000字节): " << Qt::hex << crc;
}

void TestCrc16::testCrc16Consistency()
{
    // 验证相同数据总是产生相同 CRC
    QByteArray data = QByteArray::fromHex("01030000000A");
    quint16 crc1 = Crc16::calculate(data);
    quint16 crc2 = Crc16::calculate(data);
    quint16 crc3 = Crc16::calculate(data);
    
    QCOMPARE(crc1, crc2);
    QCOMPARE(crc2, crc3);
    qDebug() << "CRC 一致性验证通过";
}
