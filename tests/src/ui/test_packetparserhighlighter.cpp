#include "test_packetparserhighlighter.h"
#include "ui/packetparserhighlighter.h"
#include "protocol/crc16.h"

#include <QtTest>
#include <QByteArray>
#include <QDebug>

void TestPacketParserHighlighter::initTestCase() {}
void TestPacketParserHighlighter::cleanupTestCase() {}

// ---- 辅助函数：构造合法报文 ----

QByteArray TestPacketParserHighlighter::makeRtuReadRequest(quint8 slave, quint16 start, quint16 qty)
{
    QByteArray frame;
    frame.append(static_cast<char>(slave));
    frame.append(static_cast<char>(0x03));
    frame.append(static_cast<char>((start >> 8) & 0xFF));
    frame.append(static_cast<char>(start & 0xFF));
    frame.append(static_cast<char>((qty >> 8) & 0xFF));
    frame.append(static_cast<char>(qty & 0xFF));
    quint16 crc = Crc16::calculate(frame);
    frame.append(static_cast<char>(crc & 0xFF));        // 低字节在前
    frame.append(static_cast<char>((crc >> 8) & 0xFF)); // 高字节在后
    return frame;
}

QByteArray TestPacketParserHighlighter::makeTcpReadRequest(quint16 tid, quint8 unit, quint8 fc,
                                                            quint16 start, quint16 qty)
{
    QByteArray frame;
    frame.append(static_cast<char>((tid >> 8) & 0xFF));
    frame.append(static_cast<char>(tid & 0xFF));
    frame.append(static_cast<char>(0x00)); // PID 高
    frame.append(static_cast<char>(0x00)); // PID 低
    // Length = 6 (unit + fc + start + qty)
    frame.append(static_cast<char>(0x00));
    frame.append(static_cast<char>(0x06));
    frame.append(static_cast<char>(unit));
    frame.append(static_cast<char>(fc));
    frame.append(static_cast<char>((start >> 8) & 0xFF));
    frame.append(static_cast<char>(start & 0xFF));
    frame.append(static_cast<char>((qty >> 8) & 0xFF));
    frame.append(static_cast<char>(qty & 0xFF));
    return frame;
}

QByteArray TestPacketParserHighlighter::makeBinaryFrame(const QByteArray &payload)
{
    QByteArray frame;
    frame.append(static_cast<char>(0xAA));
    frame.append(static_cast<char>(0x55));
    // Length (小端)
    int len = payload.size();
    frame.append(static_cast<char>(len & 0xFF));
    frame.append(static_cast<char>((len >> 8) & 0xFF));
    frame.append(payload);
    quint16 crc = Crc16::calculate(payload);
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(0x0D));
    frame.append(static_cast<char>(0x0A));
    return frame;
}

// ---- hexStringToBytes ----

void TestPacketParserHighlighter::testHexStringToBytesNormal()
{
    QByteArray bytes = PacketParserHighlighter::hexStringToBytes("01 03 00 00 00 02");
    QCOMPARE(bytes.size(), 6);
    QCOMPARE((quint8)bytes[0], 0x01);
    QCOMPARE((quint8)bytes[1], 0x03);
    QCOMPARE((quint8)bytes[5], 0x02);
}

void TestPacketParserHighlighter::testHexStringToBytesEmbedded()
{
    // 嵌入在日志消息中
    QString log = "[INFO] 发送: 01 03 00 00 00 02 C4 0B 完成";
    QByteArray bytes = PacketParserHighlighter::hexStringToBytes(log);
    QVERIFY(!bytes.isEmpty());
    QCOMPARE(bytes.size(), 8);
    QCOMPARE((quint8)bytes[0], 0x01);
}

void TestPacketParserHighlighter::testHexStringToBytesSingleByte()
{
    // 单字节不匹配（需要 >= 2 个字节段）
    QByteArray bytes = PacketParserHighlighter::hexStringToBytes("AB");
    QVERIFY(bytes.isEmpty());
}

void TestPacketParserHighlighter::testHexStringToBytesInvalid()
{
    // 纯非法 hex（无连续 2 字符 hex 段）
    QByteArray bytes = PacketParserHighlighter::hexStringToBytes("ZZ YY XX");
    QVERIFY(bytes.isEmpty());
}

void TestPacketParserHighlighter::testHexStringToBytesEmpty()
{
    QByteArray bytes = PacketParserHighlighter::hexStringToBytes("");
    QVERIFY(bytes.isEmpty());
}

// ---- detectProto ----

void TestPacketParserHighlighter::testDetectProtoModbusRtu()
{
    QByteArray frame = makeRtuReadRequest(1, 0, 2);
    QCOMPARE(PacketParserHighlighter::detectProto(frame),
             PacketParserHighlighter::ModbusRtu);
}

void TestPacketParserHighlighter::testDetectProtoModbusTcp()
{
    QByteArray frame = makeTcpReadRequest(1, 1, 0x03, 0, 2);
    QCOMPARE(PacketParserHighlighter::detectProto(frame),
             PacketParserHighlighter::ModbusTcp);
}

void TestPacketParserHighlighter::testDetectProtoBinaryFrame()
{
    QByteArray payload;
    payload.append(static_cast<char>(0x01));
    payload.append(static_cast<char>(0x02));
    QByteArray frame = makeBinaryFrame(payload);
    QCOMPARE(PacketParserHighlighter::detectProto(frame),
             PacketParserHighlighter::BinaryFrame);
}

void TestPacketParserHighlighter::testDetectProtoUnknown()
{
    QByteArray garbage;
    garbage.append("hello");
    QCOMPARE(PacketParserHighlighter::detectProto(garbage),
             PacketParserHighlighter::Unknown);
}

// ---- detectFromString ----

void TestPacketParserHighlighter::testDetectFromStringRtu()
{
    QByteArray frame = makeRtuReadRequest(1, 0, 2);
    // 转成 hex 字符串
    QString hex;
    for (int i = 0; i < frame.size(); ++i) {
        if (i > 0) hex += " ";
        hex += QString::number((quint8)frame[i], 16).toUpper().rightJustified(2, '0');
    }
    QByteArray outRaw;
    auto proto = PacketParserHighlighter::detectFromString(hex, &outRaw);
    QCOMPARE(proto, PacketParserHighlighter::ModbusRtu);
    QCOMPARE(outRaw, frame);
}

// ---- parseFieldTable: Modbus RTU ----

void TestPacketParserHighlighter::testParseRtuReadRequest()
{
    QByteArray frame = makeRtuReadRequest(1, 0, 2);
    auto rows = PacketParserHighlighter::parseFieldTable(
        frame, PacketParserHighlighter::ModbusRtu);
    QVERIFY(!rows.isEmpty());
    // 应有: SlaveAddr, FunctionCode, StartAddr, Quantity, CRC16
    QCOMPARE(rows.size(), 5);
    QCOMPARE(rows[0].fieldName, QString("SlaveAddr"));
    QCOMPARE(rows[0].dec, QString("1"));
    QCOMPARE(rows[1].fieldName, QString("FunctionCode"));
    QCOMPARE(rows[2].fieldName, QString("StartAddr"));
    QCOMPARE(rows[2].dec, QString("0"));
    QCOMPARE(rows[3].fieldName, QString("Quantity"));
    QCOMPARE(rows[3].dec, QString("2"));
    QCOMPARE(rows[4].fieldName, QString("CRC16"));
    QVERIFY(rows[4].valid); // CRC 校验通过
}

void TestPacketParserHighlighter::testParseRtuReadResponse()
{
    // 构造读响应: slave=1, fc=3, byteCount=4, data=0005 0006
    QByteArray resp;
    resp.append(static_cast<char>(0x01));
    resp.append(static_cast<char>(0x03));
    resp.append(static_cast<char>(0x04)); // byte count
    resp.append(static_cast<char>(0x00)); resp.append(static_cast<char>(0x05));
    resp.append(static_cast<char>(0x00)); resp.append(static_cast<char>(0x06));
    quint16 crc = Crc16::calculate(resp);
    resp.append(static_cast<char>(crc & 0xFF));
    resp.append(static_cast<char>((crc >> 8) & 0xFF));

    auto rows = PacketParserHighlighter::parseFieldTable(
        resp, PacketParserHighlighter::ModbusRtu);
    QVERIFY(rows.size() >= 4);
    // SlaveAddr, FunctionCode, ByteCount, Data, CRC16
    bool foundData = false;
    bool foundByteCount = false;
    for (const auto &r : rows) {
        if (r.fieldName == "ByteCount") { foundByteCount = true; QCOMPARE(r.dec, QString("4")); }
        if (r.fieldName == "Data") { foundData = true; QCOMPARE(r.dec, QString("2 reg(s)")); }
    }
    QVERIFY(foundByteCount);
    QVERIFY(foundData);
}

void TestPacketParserHighlighter::testParseRtuWriteSingleRegister()
{
    // 写单寄存器: slave=1, fc=6, addr=10, value=0x1234
    QByteArray frame;
    frame.append(static_cast<char>(0x01));
    frame.append(static_cast<char>(0x06));
    frame.append(static_cast<char>(0x00)); frame.append(static_cast<char>(0x0A));
    frame.append(static_cast<char>(0x12)); frame.append(static_cast<char>(0x34));
    quint16 crc = Crc16::calculate(frame);
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));

    auto rows = PacketParserHighlighter::parseFieldTable(
        frame, PacketParserHighlighter::ModbusRtu);
    // SlaveAddr, FunctionCode, Addr, Value, CRC16
    QCOMPARE(rows.size(), 5);
    bool foundAddr = false, foundValue = false;
    for (const auto &r : rows) {
        if (r.fieldName == "Addr") { foundAddr = true; QCOMPARE(r.dec, QString("10")); }
        if (r.fieldName == "Value") { foundValue = true; QCOMPARE(r.dec, QString("4660")); } // 0x1234
    }
    QVERIFY(foundAddr);
    QVERIFY(foundValue);
}

void TestPacketParserHighlighter::testParseRtuCrcValid()
{
    QByteArray frame = makeRtuReadRequest(1, 0, 2);
    auto rows = PacketParserHighlighter::parseFieldTable(
        frame, PacketParserHighlighter::ModbusRtu);
    // 最后一行是 CRC16，应 valid=true
    QVERIFY(!rows.isEmpty());
    const auto &crcRow = rows.last();
    QCOMPARE(crcRow.fieldName, QString("CRC16"));
    QVERIFY(crcRow.valid);
    QCOMPARE(crcRow.dec, QString("OK"));

    // 破坏 CRC 后应 valid=false
    QByteArray bad = frame;
    bad[bad.size()-1] = static_cast<char>(0xFF);
    auto badRows = PacketParserHighlighter::parseFieldTable(
        bad, PacketParserHighlighter::ModbusRtu);
    QVERIFY(!badRows.isEmpty());
    QCOMPARE(badRows.last().valid, false);
}

// ---- parseFieldTable: Modbus TCP ----

void TestPacketParserHighlighter::testParseTcpReadRequest()
{
    QByteArray frame = makeTcpReadRequest(1, 1, 0x03, 0, 2);
    auto rows = PacketParserHighlighter::parseFieldTable(
        frame, PacketParserHighlighter::ModbusTcp);
    QVERIFY(!rows.isEmpty());
    // 应含: MBAP, TransactionId, ProtocolId, Length, UnitId, FunctionCode, StartAddr, Quantity
    bool foundTid = false, foundLen = false, foundStart = false;
    for (const auto &r : rows) {
        if (r.fieldName == "TransactionId") { foundTid = true; QCOMPARE(r.dec, QString("1")); }
        if (r.fieldName == "Length") { foundLen = true; QCOMPARE(r.dec, QString("6")); QVERIFY(r.valid); }
        if (r.fieldName == "StartAddr") { foundStart = true; QCOMPARE(r.dec, QString("0")); }
    }
    QVERIFY(foundTid);
    QVERIFY(foundLen);
    QVERIFY(foundStart);
}

void TestPacketParserHighlighter::testParseTcpReadResponse()
{
    // 读响应: tid=1, unit=1, fc=3, byteCount=4, data=0005 0006
    QByteArray resp;
    resp.append(static_cast<char>(0x00)); resp.append(static_cast<char>(0x01)); // TID
    resp.append(static_cast<char>(0x00)); resp.append(static_cast<char>(0x00)); // PID
    resp.append(static_cast<char>(0x00)); resp.append(static_cast<char>(0x07)); // Length=7
    resp.append(static_cast<char>(0x01)); // Unit
    resp.append(static_cast<char>(0x03)); // FC
    resp.append(static_cast<char>(0x04)); // ByteCount
    resp.append(static_cast<char>(0x00)); resp.append(static_cast<char>(0x05));
    resp.append(static_cast<char>(0x00)); resp.append(static_cast<char>(0x06));

    auto rows = PacketParserHighlighter::parseFieldTable(
        resp, PacketParserHighlighter::ModbusTcp);
    bool foundData = false;
    for (const auto &r : rows) {
        if (r.fieldName == "Data") { foundData = true; QCOMPARE(r.dec, QString("2 reg(s)")); }
    }
    QVERIFY(foundData);
}

void TestPacketParserHighlighter::testParseTcpLengthField()
{
    // Length 字段 valid 校验: 正常时 valid=true
    QByteArray frame = makeTcpReadRequest(1, 1, 0x03, 0, 2);
    auto rows = PacketParserHighlighter::parseFieldTable(
        frame, PacketParserHighlighter::ModbusTcp);
    bool foundLen = false;
    for (const auto &r : rows) {
        if (r.fieldName == "Length") {
            foundLen = true;
            QVERIFY(r.valid); // 6 + 6 == 12
        }
    }
    QVERIFY(foundLen);
}

// ---- parseFieldTable: BinaryFrame ----

void TestPacketParserHighlighter::testParseBinaryFrameFields()
{
    QByteArray payload;
    payload.append(static_cast<char>(0x01));
    payload.append(static_cast<char>(0x02));
    payload.append(static_cast<char>(0x03));
    QByteArray frame = makeBinaryFrame(payload);

    auto rows = PacketParserHighlighter::parseFieldTable(
        frame, PacketParserHighlighter::BinaryFrame);
    // Header, Length, Payload, CRC16, Tail
    QCOMPARE(rows.size(), 5);
    QCOMPARE(rows[0].fieldName, QString("Header"));
    QVERIFY(rows[0].valid);
    QCOMPARE(rows[1].fieldName, QString("Length"));
    QCOMPARE(rows[1].dec, QString("3"));
    QCOMPARE(rows[2].fieldName, QString("Payload"));
    QCOMPARE(rows[3].fieldName, QString("CRC16"));
    QVERIFY(rows[3].valid);
    QCOMPARE(rows[4].fieldName, QString("Tail"));
    QVERIFY(rows[4].valid);
}

// ---- 着色渲染 ----

void TestPacketParserHighlighter::testFormatBytesAsHtml()
{
    QByteArray frame = makeRtuReadRequest(1, 0, 2);
    QString html = PacketParserHighlighter::formatBytesAsHtml(
        frame, PacketParserHighlighter::ModbusRtu);
    QVERIFY(html.contains("span"));
    QVERIFY(html.contains("background-color"));
    // 字节数应与原始字节数一致（6 数据 + 2 CRC = 8 字节）
    int spanCount = html.count("<span");
    QCOMPARE(spanCount, frame.size());
}

void TestPacketParserHighlighter::testFormatFieldTableAsText()
{
    QByteArray frame = makeRtuReadRequest(1, 0, 2);
    auto rows = PacketParserHighlighter::parseFieldTable(
        frame, PacketParserHighlighter::ModbusRtu);
    QString text = PacketParserHighlighter::formatFieldTableAsText(rows);
    QVERIFY(text.contains("Field"));
    QVERIFY(text.contains("SlaveAddr"));
    QVERIFY(text.contains("FunctionCode"));
    QVERIFY(text.contains("CRC16"));
}

// ---- protoName ----

void TestPacketParserHighlighter::testProtoName()
{
    QCOMPARE(PacketParserHighlighter::protoName(PacketParserHighlighter::BinaryFrame),
             QString("BinaryFrame"));
    QCOMPARE(PacketParserHighlighter::protoName(PacketParserHighlighter::ModbusTcp),
             QString("ModbusTcp"));
    QCOMPARE(PacketParserHighlighter::protoName(PacketParserHighlighter::ModbusRtu),
             QString("ModbusRtu"));
    QCOMPARE(PacketParserHighlighter::protoName(PacketParserHighlighter::Unknown),
             QString("Unknown"));
}
