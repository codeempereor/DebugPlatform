#include "test_icommunication.h"
#include <QCoreApplication>

void TestICommunication::initTestCase()
{
    m_comm = new TestCommunication();
    qDebug() << "=== ICommunication ?????? ===";
}

void TestICommunication::cleanupTestCase()
{
    delete m_comm;
    qDebug() << "=== ICommunication ?????? ===";
}

void TestICommunication::testConfig()
{
    CommunicationConfig config;
    config.type = CommunicationType::Tcp;
    config.tcp.hostAddress = "192.168.1.100";
    config.tcp.port = 8080;
    
    m_comm->setConfig(config);
    QCOMPARE(m_comm->config().type, CommunicationType::Tcp);
    QCOMPARE(m_comm->config().tcp.hostAddress, QString("192.168.1.100"));
    QCOMPARE(m_comm->config().tcp.port, 8080);
}

void TestICommunication::testSetConfig()
{
    CommunicationConfig config;
    config.type = CommunicationType::Serial;
    config.serial.portName = "COM3";
    config.serial.baudRate = 9600;
    
    m_comm->setConfig(config);
    QCOMPARE(m_comm->config().type, CommunicationType::Serial);
    QCOMPARE(m_comm->config().serial.portName, QString("COM3"));
    QCOMPARE(m_comm->config().serial.baudRate, 9600);
}

void TestICommunication::testOpenClose()
{
    m_comm->setOpenResult(true);
    QVERIFY(!m_comm->isOpen());
    
    QVERIFY(m_comm->open());
    QVERIFY(m_comm->isOpen());
    
    m_comm->close();
    QVERIFY(!m_comm->isOpen());
}

void TestICommunication::testSendData()
{
    m_comm->setOpenResult(true);
    m_comm->open();
    
    QByteArray data = QByteArray::fromHex("01030000000A");
    QVERIFY(m_comm->sendData(data));
    QCOMPARE(m_comm->sentData(), data);
    
    m_comm->close();
}

void TestICommunication::testSignals()
{
    QSignalSpy connectedSpy(m_comm, &ICommunication::connected);
    QSignalSpy disconnectedSpy(m_comm, &ICommunication::disconnected);
    QSignalSpy dataReceivedSpy(m_comm, &ICommunication::dataReceived);
    QSignalSpy errorSpy(m_comm, &ICommunication::errorOccurred);
    
    m_comm->setOpenResult(true);
    m_comm->open();
    QVERIFY(connectedSpy.count() >= 1);
    
    m_comm->simulateReceive(QByteArray("test data"));
    QVERIFY(dataReceivedSpy.count() >= 1);
    
    m_comm->simulateError("Test error", 42);
    QVERIFY(errorSpy.count() >= 1);
    
    m_comm->close();
    QVERIFY(disconnectedSpy.count() >= 1);
}

void TestICommunication::testNotOpen()
{
    m_comm->close();
    QVERIFY(!m_comm->isOpen());
    
    QByteArray data = QByteArray("test");
    QVERIFY(!m_comm->sendData(data));
}
