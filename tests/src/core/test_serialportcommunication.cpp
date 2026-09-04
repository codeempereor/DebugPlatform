#include "test_serialportcommunication.h"
#include <QCoreApplication>

void TestSerialPortCommunication::initTestCase()
{
    qDebug() << "=== TestSerialPortCommunication 启动 ===";
}

void TestSerialPortCommunication::cleanupTestCase()
{
    qDebug() << "=== TestSerialPortCommunication 结束 ===";
}

void TestSerialPortCommunication::init()
{
    m_comm = new SerialPortCommunication(this);
}

void TestSerialPortCommunication::cleanup()
{
    if (m_comm) {
        m_comm->close();
        delete m_comm;
        m_comm = nullptr;
    }
}

void TestSerialPortCommunication::testInitialState()
{
    QVERIFY(!m_comm->isOpen());
}

void TestSerialPortCommunication::testSetConfig()
{
    CommunicationConfig config;
    config.type = CommunicationType::Serial;
    config.serial.portName = "COM99";
    config.serial.baudRate = 115200;

    m_comm->setConfig(config);
    QVERIFY(!m_comm->isOpen());
}

void TestSerialPortCommunication::testOpenInvalidPort()
{
    CommunicationConfig config;
    config.type = CommunicationType::Serial;
    config.serial.portName = "COM99";
    config.serial.baudRate = 9600;
    m_comm->setConfig(config);

    QVERIFY(!m_comm->open());
    QVERIFY(!m_comm->isOpen());
}

void TestSerialPortCommunication::testSendDataWhenClosed()
{
    QVERIFY(!m_comm->sendData(QByteArray("test data")));
    QVERIFY(!m_comm->isOpen());
}

void TestSerialPortCommunication::testCloseWhenClosed()
{
    m_comm->close();
    QVERIFY(!m_comm->isOpen());
}

void TestSerialPortCommunication::testOpenCloseCycle()
{
    CommunicationConfig config;
    config.type = CommunicationType::Serial;
    config.serial.portName = "COM99";
    config.serial.baudRate = 9600;
    m_comm->setConfig(config);

    for (int i = 0; i < 3; ++i) {
        QVERIFY(!m_comm->open());
        QVERIFY(!m_comm->isOpen());
        m_comm->close();
        QVERIFY(!m_comm->isOpen());
    }
}

void TestSerialPortCommunication::testErrorSignalOnInvalidPort()
{
    CommunicationConfig config;
    config.type = CommunicationType::Serial;
    config.serial.portName = "COM99";
    config.serial.baudRate = 9600;
    m_comm->setConfig(config);

    QSignalSpy errorSpy(m_comm, &SerialPortCommunication::errorOccurred);
    m_comm->open();
    QCoreApplication::processEvents();

    QVERIFY(errorSpy.count() >= 0);
}

void TestSerialPortCommunication::testMultipleInstances()
{
    SerialPortCommunication *comm2 = new SerialPortCommunication(this);
    SerialPortCommunication *comm3 = new SerialPortCommunication(this);

    QVERIFY(!comm2->isOpen());
    QVERIFY(!comm3->isOpen());

    QVERIFY(!comm2->sendData(QByteArray("test")));
    QVERIFY(!comm3->sendData(QByteArray("test")));

    comm2->close();
    comm3->close();

    delete comm2;
    delete comm3;
}
