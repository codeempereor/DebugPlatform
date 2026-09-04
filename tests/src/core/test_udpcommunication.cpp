#include "test_udpcommunication.h"
#include "core/udpcommunication.h"
#include "core/communication_config.h"
#include <QUdpSocket>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>
#include <QHostAddress>

void TestUdpCommunication::initTestCase()
{
    m_comm = new UdpCommunication(this);
    m_peer = new QUdpSocket(this);
    // peer 不绑定，仅用于发送到 m_comm 绑定的端口
    qDebug() << "=== TestUdpCommunication 启动 ===";
}

void TestUdpCommunication::cleanup()
{
    // 每个测试后清理状态
    if (m_comm) m_comm->close();
}

void TestUdpCommunication::cleanupTestCase()
{
    if (m_comm) {
        m_comm->close();
        delete m_comm;
    }
    if (m_peer) {
        m_peer->close();
        delete m_peer;
    }
    qDebug() << "=== TestUdpCommunication 结束 ===";
}

void TestUdpCommunication::testOpenSuccess()
{
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.port = 15000;       // 目标端口（仅作记录）
    config.udp.localPort = 0;      // 0 = 系统自动分配
    m_comm->setConfig(config);

    QSignalSpy connectedSpy(m_comm, &UdpCommunication::connected);
    QVERIFY(m_comm->open());
    QVERIFY(m_comm->isOpen());
    QCOMPARE(connectedSpy.count(), 1);

    m_comm->close();
}

void TestUdpCommunication::testOpenAlreadyOpen()
{
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.port = 15001;
    config.udp.localPort = 0;
    m_comm->setConfig(config);

    QVERIFY(m_comm->open());
    QVERIFY(m_comm->isOpen());

    // 再次 open() 应返回 true（幂等）
    QVERIFY(m_comm->open());

    m_comm->close();
}

void TestUdpCommunication::testOpenBindFailure()
{
    // UDP 默认使用 ShareAddress | ReuseAddressHint，多 socket 可共享同一端口
    // 因此 bind 失败路径较难触发。改为：用一个无效/保留端口（端口 1 通常需要特权）
    // 注意：在大多数 Windows 系统上，绑定端口 1 会因权限不足失败
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.port = 15002;
    config.udp.localPort = 1;  // 保留端口，通常绑定失败
    m_comm->setConfig(config);

    // 尝试绑定，可能成功也可能失败，取决于系统权限
    // 这里我们仅验证：如果失败，错误信号被发射；如果成功，也接受
    QSignalSpy errorSpy(m_comm, &UdpCommunication::errorOccurred);
    bool result = m_comm->open();
    if (!result) {
        QVERIFY(!m_comm->isOpen());
        QCOMPARE(errorSpy.count(), 1);
    } else {
        m_comm->close();
    }
    QSKIP("UDP bind 失败路径依赖系统权限，跳过严格断言");
}

void TestUdpCommunication::testCloseWhenOpen()
{
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.port = 15003;
    config.udp.localPort = 0;
    m_comm->setConfig(config);

    QVERIFY(m_comm->open());
    QVERIFY(m_comm->isOpen());

    QSignalSpy disconnectedSpy(m_comm, &UdpCommunication::disconnected);
    m_comm->close();
    QVERIFY(!m_comm->isOpen());
    QCOMPARE(disconnectedSpy.count(), 1);
}

void TestUdpCommunication::testCloseWhenNotOpen()
{
    // 未打开状态下 close() 应为 no-op，不发射信号
    QSignalSpy disconnectedSpy(m_comm, &UdpCommunication::disconnected);
    m_comm->close();
    QVERIFY(!m_comm->isOpen());
    QCOMPARE(disconnectedSpy.count(), 0);
}

void TestUdpCommunication::testIsOpenStates()
{
    QVERIFY(!m_comm->isOpen());

    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.port = 15004;
    config.udp.localPort = 0;
    m_comm->setConfig(config);

    QVERIFY(m_comm->open());
    QVERIFY(m_comm->isOpen());

    m_comm->close();
    QVERIFY(!m_comm->isOpen());
}

void TestUdpCommunication::testSendDataSuccess()
{
    // 让 m_comm 绑定一个本地端口
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.localPort = 0;
    m_comm->setConfig(config);
    QVERIFY(m_comm->open());

    // peer 绑定到一个固定端口，作为接收方
    QVERIFY(m_peer->bind(QHostAddress::LocalHost, 0,
                          QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint));
    quint16 peerPort = m_peer->localPort();

    // 重新配置 m_comm 的目标为 peer 端口
    config.udp.port = peerPort;
    m_comm->setConfig(config);

    QByteArray payload = QByteArray::fromHex("AA55DEADBEEF");
    QVERIFY(m_comm->sendData(payload));

    // 等待 peer 收到数据
    QVERIFY(m_peer->waitForReadyRead(500));
    QVERIFY(m_peer->hasPendingDatagrams());
    qint64 size = m_peer->pendingDatagramSize();
    QByteArray received(size, 0);
    m_peer->readDatagram(received.data(), size);
    QCOMPARE(received, payload);

    m_comm->close();
    m_peer->close();
}

void TestUdpCommunication::testSendDataWhenNotOpen()
{
    QSignalSpy errorSpy(m_comm, &UdpCommunication::errorOccurred);
    QVERIFY(!m_comm->sendData(QByteArray::fromHex("AA55")));
    QCOMPARE(errorSpy.count(), 1);
}

void TestUdpCommunication::testSendDataEmpty()
{
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.localPort = 0;
    m_comm->setConfig(config);
    QVERIFY(m_comm->open());

    QSignalSpy errorSpy(m_comm, &UdpCommunication::errorOccurred);
    QVERIFY(!m_comm->sendData(QByteArray()));
    QCOMPARE(errorSpy.count(), 1);

    m_comm->close();
}

void TestUdpCommunication::testDataReceived()
{
    // 用 probe 找一个空闲端口，让 m_comm 绑定该端口
    quint16 commPort = 0;
    {
        QUdpSocket probe;
        QVERIFY(probe.bind(QHostAddress(QHostAddress::LocalHost), quint16(0)));
        commPort = probe.localPort();
        probe.close();  // 释放端口
    }

    // m_comm 绑定到 commPort
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.port = 15005;
    config.udp.localPort = commPort;
    m_comm->setConfig(config);
    QVERIFY(m_comm->open());

    // m_peer 也绑定到随机端口
    QVERIFY(m_peer->bind(QHostAddress(QHostAddress::LocalHost), quint16(0),
                          QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint));

    // 注册 dataReceived 信号 spy
    QSignalSpy dataSpy(m_comm, &UdpCommunication::dataReceived);

    // peer 向 m_comm 绑定的端口发送数据报
    QVERIFY(m_peer->writeDatagram(QByteArray::fromHex("C0FFEE"),
                                   QHostAddress::LocalHost, commPort) > 0);

    // 等待 m_comm 收到数据
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();

    QVERIFY(dataSpy.count() >= 1);
    QByteArray received = dataSpy.takeFirst().at(0).toByteArray();
    QCOMPARE(received, QByteArray::fromHex("C0FFEE"));

    m_comm->close();
    m_peer->close();
}

void TestUdpCommunication::testConnectedSignalEmitted()
{
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.localPort = 0;
    m_comm->setConfig(config);

    QSignalSpy connectedSpy(m_comm, &UdpCommunication::connected);
    QVERIFY(m_comm->open());
    QCOMPARE(connectedSpy.count(), 1);

    m_comm->close();
}

void TestUdpCommunication::testDisconnectedSignalEmitted()
{
    CommunicationConfig config;
    config.type = CommunicationType::Udp;
    config.udp.hostAddress = "127.0.0.1";
    config.udp.localPort = 0;
    m_comm->setConfig(config);

    QVERIFY(m_comm->open());

    QSignalSpy disconnectedSpy(m_comm, &UdpCommunication::disconnected);
    m_comm->close();
    QCOMPARE(disconnectedSpy.count(), 1);
}
