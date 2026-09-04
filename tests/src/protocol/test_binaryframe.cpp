#include "test_binaryframe.h"
#include "protocol/binaryframebuilder.h"
#include "protocol/binaryframeparser.h"
#include "protocol/crc16.h"
#include <QSignalSpy>

void TestBinaryFrame::initTestCase()
{
    qDebug() << "=== BinaryFrame 测试套件开始 ===";
}

void TestBinaryFrame::cleanupTestCase()
{
    qDebug() << "=== BinaryFrame 测试套件结束 ===";
}

void TestBinaryFrame::testBuildFrameEmptyPayload()
{
    QByteArray emptyPayload;
    QByteArray frame = BinaryFrameBuilder::buildFrame(emptyPayload);
    
    QCOMPARE(frame.size(), 8);
    
    QCOMPARE(static_cast<unsigned char>(frame[0]), 0xAA);
    QCOMPARE(static_cast<unsigned char>(frame[1]), 0x55);
    
    QCOMPARE(static_cast<unsigned char>(frame[frame.size()-2]), 0x0D);
    QCOMPARE(static_cast<unsigned char>(frame[frame.size()-1]), 0x0A);
    
    qDebug() << "空负载帧: " << frame.toHex(' ');
}

void TestBinaryFrame::testBuildFrameWithData()
{
    QByteArray payload = QByteArray::fromHex("000242F6E979");
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
    
    QCOMPARE(frame.size(), 14);
    
    int length = static_cast<unsigned char>(frame[2]) | 
                 (static_cast<unsigned char>(frame[3]) << 8);
    QCOMPARE(length, 6);
    
    qDebug() << "数据帧: " << frame.toHex(' ');
}

void TestBinaryFrame::testBuildFrameFloatValue()
{
    quint16 addr = 2;
    float value = 123.456f;
    
    QByteArray payload;
    payload.append(static_cast<char>(addr & 0xFF));
    payload.append(static_cast<char>((addr >> 8) & 0xFF));
    
    const char* floatBytes = reinterpret_cast<const char*>(&value);
    for (int i = 0; i < 4; ++i) {
        payload.append(floatBytes[i]);
    }
    
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
    qDebug() << "Float 测试帧: " << frame.toHex(' ');
    
    BinaryFrameParser parser;
    QSignalSpy spy(&parser, &BinaryFrameParser::frameReceived);
    
    parser.feedData(frame);
    
    QVERIFY(spy.count() >= 1);
    
    QByteArray parsedPayload = spy.takeFirst().at(0).toByteArray();
    QCOMPARE(parsedPayload.size(), 6);
    
    quint16 parsedAddr = static_cast<unsigned char>(parsedPayload[0]) |
                         (static_cast<unsigned char>(parsedPayload[1]) << 8);
    QCOMPARE(parsedAddr, 2);
    
    float parsedValue;
    memcpy(&parsedValue, parsedPayload.constData() + 2, 4);
    QVERIFY(qFuzzyCompare(parsedValue, 123.456f));
    
    qDebug() << "解析的地址:" << parsedAddr << ", 值:" << parsedValue;
}

void TestBinaryFrame::testBuildFrameStructure()
{
    QByteArray payload = QByteArray::fromHex("000141200000");
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
    
    QVERIFY(frame.startsWith(QByteArray::fromHex("AA55")));
    QVERIFY(frame.endsWith(QByteArray::fromHex("0D0A")));
    
    qDebug() << "帧结构验证通过";
}

void TestBinaryFrame::testParseValidFrame()
{
    QByteArray payload = QByteArray::fromHex("000141200000");
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
    
    BinaryFrameParser parser;
    QSignalSpy spy(&parser, &BinaryFrameParser::frameReceived);
    
    parser.feedData(frame);
    
    QVERIFY(spy.count() >= 1);
    
    QByteArray parsedPayload = spy.takeFirst().at(0).toByteArray();
    QCOMPARE(parsedPayload, payload);
    
    qDebug() << "有效帧解析通过";
}

void TestBinaryFrame::testParseInvalidHeader()
{
    QByteArray badFrame = QByteArray::fromHex("AA660400000102030D0A");
    
    BinaryFrameParser parser;
    QSignalSpy spy(&parser, &BinaryFrameParser::parseError);
    
    parser.feedData(badFrame);
    
    QVERIFY(spy.count() >= 1);
    
    qDebug() << "无效帧头验证通过";
}

void TestBinaryFrame::testParseInvalidCrc()
{
    QByteArray payload = QByteArray::fromHex("000141200000");
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
    
    int crcIndex = frame.size() - 4;
    frame[crcIndex] = frame[crcIndex] ^ 0xFF;
    
    BinaryFrameParser parser;
    QSignalSpy spyError(&parser, &BinaryFrameParser::parseError);
    QSignalSpy spyFrame(&parser, &BinaryFrameParser::frameReceived);
    
    parser.feedData(frame);
    
    QVERIFY(spyError.count() >= 1);
    QCOMPARE(spyFrame.count(), 0);
    
    qDebug() << "无效 CRC 验证通过";
}

void TestBinaryFrame::testParseFloatPayload()
{
    QByteArray payload = QByteArray::fromHex("000242F6E979");
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
    
    BinaryFrameParser parser;
    QSignalSpy spy(&parser, &BinaryFrameParser::frameReceived);
    
    parser.feedData(frame);
    
    QVERIFY(spy.count() >= 1);
    
    QByteArray parsedPayload = spy.takeFirst().at(0).toByteArray();
    QCOMPARE(parsedPayload, payload);
    
    qDebug() << "Float 负载解析通过";
}

void TestBinaryFrame::testParsePartialFrame()
{
    QByteArray incompleteFrame = QByteArray::fromHex("AA55060000");
    
    BinaryFrameParser parser;
    QSignalSpy spyFrame(&parser, &BinaryFrameParser::frameReceived);
    
    parser.feedData(incompleteFrame);
    
    QCOMPARE(spyFrame.count(), 0);
    
    qDebug() << "不完整帧验证通过";
}

void TestBinaryFrame::testBuildThenParse()
{
    struct TestCase {
        QByteArray payload;
        const char* description;
    };
    
    TestCase cases[] = {
        { QByteArray::fromHex("000141200000"), "float 2.0" },
        { QByteArray::fromHex("000242F6E979"), "float 123.456" },
        { QByteArray::fromHex("000000000000"), "float 0.0" },
        { QByteArray::fromHex("000344800000"), "float 1024.0" },
    };
    
    for (const auto& tc : cases) {
        QByteArray frame = BinaryFrameBuilder::buildFrame(tc.payload);
        
        BinaryFrameParser parser;
        QSignalSpy spy(&parser, &BinaryFrameParser::frameReceived);
        
        parser.feedData(frame);
        
        QVERIFY2(spy.count() >= 1, tc.description);
        
        QByteArray parsedPayload = spy.takeFirst().at(0).toByteArray();
        QCOMPARE(parsedPayload, tc.payload);
        
        qDebug() << tc.description << ": 通过";
    }
}

void TestBinaryFrame::testBuildParseRoundtrip()
{
    for (int len = 0; len <= 15; ++len) {
        QByteArray payload(len, static_cast<char>(len + 1));
        
        QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
        
        BinaryFrameParser parser;
        QSignalSpy spy(&parser, &BinaryFrameParser::frameReceived);
        
        parser.feedData(frame);
        
        QVERIFY(spy.count() >= 1);
        
        QByteArray parsedPayload = spy.takeFirst().at(0).toByteArray();
        QCOMPARE(parsedPayload, payload);
        
        qDebug() << "负载长度" << len << "的往返测试通过";
    }
}
// ============================================================================
// P1-3a 错误路径测试
// ============================================================================

void TestBinaryFrame::testParseTruncatedHeader()
{
    // 仅传入 0xAA 单字节，不应触发 frameReceived，也不应崩溃
    BinaryFrameParser parser;
    QSignalSpy spyFrame(&parser, &BinaryFrameParser::frameReceived);
    QSignalSpy spyError(&parser, &BinaryFrameParser::parseError);

    parser.feedData(QByteArray::fromHex("AA"));
    QCOMPARE(spyFrame.count(), 0);
    QCOMPARE(spyError.count(), 0);  // 单字节不应立即报错，等待下一字节

    qDebug() << "截断帧头验证通过";
}

void TestBinaryFrame::testParseMissingTail()
{
    // 构造一个完整帧但去掉末尾的 0x0D 0x0A
    QByteArray payload = QByteArray::fromHex("000141200000");
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);
    QByteArray noTail = frame.left(frame.size() - 2);  // 去掉最后两字节

    BinaryFrameParser parser;
    QSignalSpy spyFrame(&parser, &BinaryFrameParser::frameReceived);
    QSignalSpy spyError(&parser, &BinaryFrameParser::parseError);

    parser.feedData(noTail);
    // 缺少帧尾，不应触发 frameReceived
    QCOMPARE(spyFrame.count(), 0);

    qDebug() << "缺少帧尾验证通过";
}

void TestBinaryFrame::testParseGarbagePrefix()
{
    // 帧前有垃圾字节，后跟合法帧，应能从垃圾中恢复并解析出合法帧
    QByteArray payload = QByteArray::fromHex("000141200000");
    QByteArray validFrame = BinaryFrameBuilder::buildFrame(payload);
    QByteArray garbage = QByteArray::fromHex("FF1122334455");
    QByteArray combined = garbage + validFrame;

    BinaryFrameParser parser;
    QSignalSpy spyFrame(&parser, &BinaryFrameParser::frameReceived);

    parser.feedData(combined);
    QVERIFY(spyFrame.count() >= 1);

    QByteArray parsedPayload = spyFrame.takeFirst().at(0).toByteArray();
    QCOMPARE(parsedPayload, payload);

    qDebug() << "垃圾前缀恢复验证通过";
}

void TestBinaryFrame::testParseMultipleFramesAtOnce()
{
    // 模拟 TCP 粘包：两个完整帧一次性 feedData，应解析出两个 frameReceived
    QByteArray payload1 = QByteArray::fromHex("000141200000");
    QByteArray payload2 = QByteArray::fromHex("000242F6E979");
    QByteArray frame1 = BinaryFrameBuilder::buildFrame(payload1);
    QByteArray frame2 = BinaryFrameBuilder::buildFrame(payload2);
    QByteArray stuck = frame1 + frame2;

    BinaryFrameParser parser;
    QSignalSpy spy(&parser, &BinaryFrameParser::frameReceived);

    parser.feedData(stuck);
    QCOMPARE(spy.count(), 2);

    qDebug() << "粘包多帧解析验证通过";
}

void TestBinaryFrame::testParseFrameSplitAcrossFeeds()
{
    // 模拟 TCP 半包：一个完整帧分多次 feedData，应能正确拼装并解析
    QByteArray payload = QByteArray::fromHex("000141200000");
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);

    BinaryFrameParser parser;
    QSignalSpy spy(&parser, &BinaryFrameParser::frameReceived);

    // 分 3 次喂入
    int split1 = 3;
    int split2 = frame.size() - 5;
    parser.feedData(frame.left(split1));
    QCOMPARE(spy.count(), 0);

    parser.feedData(frame.mid(split1, split2 - split1));
    QCOMPARE(spy.count(), 0);

    parser.feedData(frame.mid(split2));
    QVERIFY(spy.count() >= 1);

    QByteArray parsedPayload = spy.takeFirst().at(0).toByteArray();
    QCOMPARE(parsedPayload, payload);

    qDebug() << "半包分片重组验证通过";
}

void TestBinaryFrame::testParseLengthExceedsMax()
{
    // 构造一个 length 字段超过 MAX_FRAME_SIZE 的非法帧
    // 帧头: AA55, length=0xFF00 (远超 MAX_FRAME_SIZE), 后跟任意字节
    QByteArray badFrame = QByteArray::fromHex("AA5500FF");
    // 补充一些数据让解析器继续读
    badFrame.append(QByteArray(10, 0x00));

    BinaryFrameParser parser;
    QSignalSpy spyError(&parser, &BinaryFrameParser::parseError);
    QSignalSpy spyFrame(&parser, &BinaryFrameParser::frameReceived);

    parser.feedData(badFrame);
    QVERIFY(spyError.count() >= 1);
    QCOMPARE(spyFrame.count(), 0);

    qDebug() << "超长 length 字段验证通过";
}
