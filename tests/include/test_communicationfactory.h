#ifndef TEST_COMMUNICATIONFACTORY_H
#define TEST_COMMUNICATIONFACTORY_H

#include <QObject>
#include <QtTest/QtTest>
#include "core/communicationfactory.h"
#include "core/icommunication.h"

class TestCommunicationFactory : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testCreateSerial();
    void testCreateTcp();
    void testCreateUdp();
    void testCreateUnknown();
    void testRegisterCreator();
    void testUnregisterCreator();
    void testIsRegistered();
    void testRegisteredTypes();
    
private:
    CommunicationConfig m_serialConfig;
    CommunicationConfig m_tcpConfig;
    CommunicationConfig m_udpConfig;
};

#endif
