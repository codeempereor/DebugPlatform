#ifndef TEST_PLAYBACKENGINE_H
#define TEST_PLAYBACKENGINE_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "data/playbackengine.h"

class TestPlaybackEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testLoadFile();
    void testLoadInvalidFile();
    void testStepForward();
    void testStepBackward();
    void testSetSpeed();
    void testStateTransitions();
    void testIntervalCalculation();
    
private:
    QString m_testDataDir;
    QString createTestFile();
};

#endif
