#ifndef TEST_SERIALPORTCOMMUNICATION_H
#define TEST_SERIALPORTCOMMUNICATION_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "core/serialportcommunication.h"
#include "core/communication_config.h"

class TestSerialPortCommunication : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testInitialState();
    void testSetConfig();
    void testOpenInvalidPort();
    void testSendDataWhenClosed();
    void testCloseWhenClosed();
    void testOpenCloseCycle();
    void testErrorSignalOnInvalidPort();
    void testMultipleInstances();

private:
    SerialPortCommunication *m_comm = nullptr;
};

#endif
