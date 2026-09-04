#ifndef TEST_LOGMANAGER_H
#define TEST_LOGMANAGER_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "data/logmanager.h"

class TestLogManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testLogLevels();
    void testLogMinLevel();
    void testLogSignal();
    void testLogFile();
    void testMaxEntries();
    void testLevelToString();
    
private:
    QString m_testLogDir;
};

#endif
