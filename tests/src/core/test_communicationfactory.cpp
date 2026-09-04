#include "test_communicationfactory.h"
#include "core/serialportcommunication.h"
#include "core/tcpcommunication.h"
#include <QSerialPort>
#include "core/udpcommunication.h"
#include <QCoreApplication>

void TestCommunicationFactory::initTestCase()
{
    m_serialConfig.type = CommunicationType::Serial;
    m_serialConfig.serial.portName = "COM9";
    m_serialConfig.serial.baudRate = QSerialPort::Baud115200;
    m_serialConfig.serial.dataBits = QSerialPort::Data8;
    m_serialConfig.serial.parity = QSerialPort::NoParity;
    m_serialConfig.serial.stopBits = QSerialPort::OneStop;
    m_serialConfig.serial.flowControl = QSerialPort::NoFlowControl;
    
    m_tcpConfig.type = CommunicationType::Tcp;
    m_tcpConfig.tcp.hostAddress = "127.0.0.1";
    m_tcpConfig.tcp.port = 502;
    
    m_udpConfig.type = CommunicationType::Udp;
    m_udpConfig.udp.hostAddress = "127.0.0.1";
    m_udpConfig.udp.port = 503;
    
    qDebug() << "=== CommunicationFactory ?????? ===";
}

void TestCommunicationFactory::cleanupTestCase()
{
    qDebug() << "=== CommunicationFactory ?????? ===";
}

void TestCommunicationFactory::testCreateSerial()
{
    ICommunication *comm = CommunicationFactory::create(CommunicationType::Serial, m_serialConfig);
    QVERIFY(comm != nullptr);
    QCOMPARE(comm->config().type, CommunicationType::Serial);
    delete comm;
}

void TestCommunicationFactory::testCreateTcp()
{
    ICommunication *comm = CommunicationFactory::create(CommunicationType::Tcp, m_tcpConfig);
    QVERIFY(comm != nullptr);
    QCOMPARE(comm->config().type, CommunicationType::Tcp);
    delete comm;
}

void TestCommunicationFactory::testCreateUdp()
{
    ICommunication *comm = CommunicationFactory::create(CommunicationType::Udp, m_udpConfig);
    QVERIFY(comm != nullptr);
    QCOMPARE(comm->config().type, CommunicationType::Udp);
    delete comm;
}

void TestCommunicationFactory::testCreateUnknown()
{
    CommunicationFactory::create(CommunicationType::Serial, m_serialConfig);
    CommunicationFactory::unregisterCreator(CommunicationType::Serial);
    
    ICommunication *comm = CommunicationFactory::create(CommunicationType::Serial, m_serialConfig);
    QVERIFY(comm == nullptr);
    
    CommunicationFactory::registerCreator(CommunicationType::Serial, []() -> ICommunication* {
        return new SerialPortCommunication();
    });
}

void TestCommunicationFactory::testRegisterCreator()
{
    bool result = CommunicationFactory::registerCreator(CommunicationType::Serial, []() -> ICommunication* {
        return new SerialPortCommunication();
    });
    QVERIFY(!result); // Already registered
}

void TestCommunicationFactory::testUnregisterCreator()
{
    bool result = CommunicationFactory::unregisterCreator(CommunicationType::Serial);
    QVERIFY(result);
    
    result = CommunicationFactory::unregisterCreator(CommunicationType::Serial);
    QVERIFY(!result);
    
    CommunicationFactory::registerCreator(CommunicationType::Serial, []() -> ICommunication* {
        return new SerialPortCommunication();
    });
}

void TestCommunicationFactory::testIsRegistered()
{
    QVERIFY(CommunicationFactory::isRegistered(CommunicationType::Serial));
    QVERIFY(CommunicationFactory::isRegistered(CommunicationType::Tcp));
    QVERIFY(CommunicationFactory::isRegistered(CommunicationType::Udp));
}

void TestCommunicationFactory::testRegisteredTypes()
{
    QList<CommunicationType> types = CommunicationFactory::registeredTypes();
    QVERIFY(types.size() >= 3);
    QVERIFY(types.contains(CommunicationType::Serial));
    QVERIFY(types.contains(CommunicationType::Tcp));
    QVERIFY(types.contains(CommunicationType::Udp));
}