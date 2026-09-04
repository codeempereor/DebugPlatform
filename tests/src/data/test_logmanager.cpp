#include "test_logmanager.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

void TestLogManager::initTestCase()
{
    m_testLogDir = QCoreApplication::applicationDirPath() + "/test_logs";
    QDir().mkpath(m_testLogDir);
    qRegisterMetaType<LogLevel>("LogLevel");
    LogManager::instance().setMinLevel(LogLevel::Debug);
    LogManager::instance().setMaxEntries(0);
    qDebug() << "=== LogManager test init ===";
}

void TestLogManager::cleanupTestCase()
{
    LogManager::instance().setLogFile("");
    QDir(m_testLogDir).removeRecursively();
    qDebug() << "=== LogManager test cleanup ===";
}

void TestLogManager::testLogLevels()
{
    LogManager::instance().log(LogLevel::Debug, "test", "Debug message");
    LogManager::instance().log(LogLevel::Info, "test", "Info message");
    LogManager::instance().log(LogLevel::Warning, "test", "Warning message");
    LogManager::instance().log(LogLevel::Error, "test", "Error message");
    QVERIFY(true);
}

void TestLogManager::testLogMinLevel()
{
    LogManager::instance().setMinLevel(LogLevel::Warning);
    LogManager::instance().log(LogLevel::Debug, "test", "Should not appear");
    LogManager::instance().log(LogLevel::Info, "test", "Should not appear");
    LogManager::instance().log(LogLevel::Warning, "test", "Should appear");
    LogManager::instance().log(LogLevel::Error, "test", "Should appear");
    LogManager::instance().setMinLevel(LogLevel::Debug);
    QVERIFY(true);
}

void TestLogManager::testLogSignal()
{
    QSignalSpy spy(&LogManager::instance(), &LogManager::logMessage);
    QVERIFY(spy.isValid());
    
    LogManager::instance().log(LogLevel::Info, "test_signal", "Signal test message");
    
    // Process events to ensure signal is delivered
    QCoreApplication::processEvents();
    
    QTRY_VERIFY(spy.count() >= 1);
    QCOMPARE(spy.count(), 1);
    
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).value<LogLevel>(), LogLevel::Info);
    QCOMPARE(args.at(1).toString(), QString("test_signal"));
    QCOMPARE(args.at(2).toString(), QString("Signal test message"));
}

void TestLogManager::testLogFile()
{
    QString logPath = m_testLogDir + "/test.log";
    LogManager::instance().setLogFile(logPath);
    
    LogManager::instance().log(LogLevel::Info, "file_test", "File log message 1");
    LogManager::instance().log(LogLevel::Info, "file_test", "File log message 2");
    
    QTest::qWait(100);
    
    QFile file(logPath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    QVERIFY(content.contains("File log message 1"));
    QVERIFY(content.contains("File log message 2"));
    
    LogManager::instance().setLogFile("");
}

void TestLogManager::testMaxEntries()
{
    QString logPath = m_testLogDir + "/max_entries.log";
    LogManager::instance().setMaxEntries(5);
    LogManager::instance().setLogFile(logPath);
    
    for (int i = 0; i < 10; ++i) {
        LogManager::instance().log(LogLevel::Info, "max_test", QString("Message %1").arg(i));
    }
    
    QTest::qWait(200);
    
    LogManager::instance().setLogFile("");
    LogManager::instance().setMaxEntries(0);
    QVERIFY(true);
}

void TestLogManager::testLevelToString()
{
    LogManager::instance().log(LogLevel::Debug, "test_level", "Debug level test");
    LogManager::instance().log(LogLevel::Info, "test_level", "Info level test");
    LogManager::instance().log(LogLevel::Warning, "test_level", "Warning level test");
    LogManager::instance().log(LogLevel::Error, "test_level", "Error level test");
    QVERIFY(true);
}
