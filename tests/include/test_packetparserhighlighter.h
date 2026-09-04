#ifndef TEST_PACKETPARSERHIGHLIGHTER_H
#define TEST_PACKETPARSERHIGHLIGHTER_H

#include <QObject>

class TestPacketParserHighlighter : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    // hexStringToBytes
    void testHexStringToBytesNormal();
    void testHexStringToBytesEmbedded();
    void testHexStringToBytesSingleByte();
    void testHexStringToBytesInvalid();
    void testHexStringToBytesEmpty();

    // detectProto
    void testDetectProtoModbusRtu();
    void testDetectProtoModbusTcp();
    void testDetectProtoBinaryFrame();
    void testDetectProtoUnknown();

    // detectFromString
    void testDetectFromStringRtu();

    // parseFieldTable - Modbus RTU
    void testParseRtuReadRequest();
    void testParseRtuReadResponse();
    void testParseRtuWriteSingleRegister();
    void testParseRtuCrcValid();

    // parseFieldTable - Modbus TCP
    void testParseTcpReadRequest();
    void testParseTcpReadResponse();
    void testParseTcpLengthField();

    // parseFieldTable - BinaryFrame
    void testParseBinaryFrameFields();

    // 着色渲染
    void testFormatBytesAsHtml();
    void testFormatFieldTableAsText();

    // protoName
    void testProtoName();

private:
    // 构造一个合法的 Modbus RTU 读保持寄存器请求帧
    QByteArray makeRtuReadRequest(quint8 slave, quint16 start, quint16 qty);
    // 构造一个合法的 Modbus TCP 读保持寄存器请求帧
    QByteArray makeTcpReadRequest(quint16 tid, quint8 unit, quint8 fc,
                                  quint16 start, quint16 qty);
    // 构造一个合法的 BinaryFrame
    QByteArray makeBinaryFrame(const QByteArray &payload);
};

#endif // TEST_PACKETPARSERHIGHLIGHTER_H
