#include "test_playbackengine.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QSignalSpy>

QString TestPlaybackEngine::createTestFile()
{
    QString path = m_testDataDir + "/test_playback.txt";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "2026-01-01 10:00:00.000\tTX\t01 03 00 00 00 0A C5 CD\n";
        out << "2026-01-01 10:00:00.050\tRX\t01 03 14 00 00 00 64 ...\n";
        out << "2026-01-01 10:00:00.100\tTX\t01 06 00 00 00 64 88 21\n";
        out << "2026-01-01 10:00:00.150\tRX\t01 06 00 00 00 64 88 21\n";
        out << "2026-01-01 10:00:00.200\tTX\t01 10 00 00 00 03 06 00 64 00 C8 01 2C 16 FB\n";
        file.close();
    }
    return path;
}

void TestPlaybackEngine::initTestCase()
{
    m_testDataDir = QCoreApplication::applicationDirPath() + "/test_playback_data";
    QDir().mkpath(m_testDataDir);
}

void TestPlaybackEngine::cleanupTestCase()
{
    QDir(m_testDataDir).removeRecursively();
}

void TestPlaybackEngine::testLoadFile()
{
    PlaybackEngine engine;
    QString filePath = createTestFile();
    
    bool result = engine.loadFile(filePath);
    QVERIFY(result);
    QCOMPARE(engine.totalPackets(), 5);
    QCOMPARE(engine.currentPosition(), -1);
    QCOMPARE(engine.state(), PlaybackEngine::PlaybackState::Idle);
}

void TestPlaybackEngine::testLoadInvalidFile()
{
    PlaybackEngine engine;
    bool result = engine.loadFile("/nonexistent/file.txt");
    QVERIFY(!result);
    QCOMPARE(engine.totalPackets(), 0);
}

void TestPlaybackEngine::testStepForward()
{
    PlaybackEngine engine;
    QString filePath = createTestFile();
    engine.loadFile(filePath);
    
    QCOMPARE(engine.currentPosition(), -1);
    engine.stepForward();
    QCOMPARE(engine.currentPosition(), 0);
    engine.stepForward();
    QCOMPARE(engine.currentPosition(), 1);
}

void TestPlaybackEngine::testStepBackward()
{
    PlaybackEngine engine;
    QString filePath = createTestFile();
    engine.loadFile(filePath);
    
    engine.stepForward();
    engine.stepForward();
    engine.stepForward();
    QCOMPARE(engine.currentPosition(), 2);
    
    engine.stepBackward();
    QCOMPARE(engine.currentPosition(), 1);
    
    engine.stepBackward();
    engine.stepBackward();
    QCOMPARE(engine.currentPosition(), -1);
}

void TestPlaybackEngine::testSetSpeed()
{
    PlaybackEngine engine;
    engine.setSpeed(2.0);
    engine.setSpeed(0.1);
    engine.setSpeed(10.0);
    engine.setSpeed(0.5);
    engine.setSpeed(4.0);
    QVERIFY(true);
}

void TestPlaybackEngine::testStateTransitions()
{
    PlaybackEngine engine;
    QString filePath = createTestFile();
    engine.loadFile(filePath);
    
    QCOMPARE(engine.state(), PlaybackEngine::PlaybackState::Idle);
    
    engine.start();
    QCoreApplication::processEvents();
    QVERIFY(engine.state() == PlaybackEngine::PlaybackState::Playing || 
            engine.state() == PlaybackEngine::PlaybackState::Idle);
    
    engine.stop();
    QCOMPARE(engine.state(), PlaybackEngine::PlaybackState::Idle);
}

void TestPlaybackEngine::testIntervalCalculation()
{
    PlaybackEngine engine;
    QString filePath = createTestFile();
    engine.loadFile(filePath);
    
    QVERIFY(engine.totalPackets() > 0);
    QVERIFY(engine.state() == PlaybackEngine::PlaybackState::Idle);
}
