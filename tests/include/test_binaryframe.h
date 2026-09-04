#ifndef TEST_BINARYFRAME_H
#define TEST_BINARYFRAME_H

#include <QObject>
#include <QtTest/QtTest>

class TestBinaryFrame : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // BinaryFrameBuilder 测试
    void testBuildFrameEmptyPayload();
    void testBuildFrameWithData();
    void testBuildFrameFloatValue();
    void testBuildFrameStructure();
    
    // BinaryFrameParser 测试
    void testParseValidFrame();
    void testParseInvalidHeader();
    void testParseInvalidCrc();
    void testParseFloatPayload();
    void testParsePartialFrame();
    
    // 集成测试
    void testBuildThenParse();
    void testBuildParseRoundtrip();

    // 错误路径测试 (P1-3a)
    void testParseTruncatedHeader();
    void testParseMissingTail();
    void testParseGarbagePrefix();
    void testParseMultipleFramesAtOnce();
    void testParseFrameSplitAcrossFeeds();
    void testParseLengthExceedsMax();
};

#endif // TEST_BINARYFRAME_H
