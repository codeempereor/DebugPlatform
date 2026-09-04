#include "core/udpcommunication.h"
#include <QDebug>

UdpCommunication::UdpCommunication(QObject *parent)
    : ICommunication(parent)
{
    // socket 在 open() 中懒创建，确保工作线程亲和性
}

UdpCommunication::~UdpCommunication()
{
    close();
    // m_udpSocket 父对象为本类，本类析构时 Qt 自动 delete
}

bool UdpCommunication::open()
{
    if (m_isOpen) {
        return true;
    }

    const UdpConfig &cfg = m_config.udp;

    // 懒创建 socket（在工作线程中创建）
    if (!m_udpSocket) {
        m_udpSocket = new QUdpSocket(this);
        connect(m_udpSocket, &QUdpSocket::readyRead,
                this, &UdpCommunication::onReadyRead);
        connect(m_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(onErrorOccurred(QAbstractSocket::SocketError)));
    }

    // 绑定本地端口用于接收数据
    if (!m_udpSocket->bind(cfg.localPort,
                           QAbstractSocket::ShareAddress
                           | QAbstractSocket::ReuseAddressHint)) {
        emit errorOccurred(tr("UDP 绑定端口失败: %1 - %2")
                           .arg(cfg.localPort)
                           .arg(m_udpSocket->errorString()),
                           static_cast<int>(m_udpSocket->error()));
        return false;
    }

    m_isOpen = true;
    emit connected();
    return true;
}

void UdpCommunication::close()
{
    if (!m_isOpen) {
        return;
    }

    if (m_udpSocket) {
        m_udpSocket->close();
    }
    m_isOpen = false;
    emit disconnected();
}

bool UdpCommunication::isOpen() const
{
    return m_isOpen && m_udpSocket && m_udpSocket->state() == QAbstractSocket::BoundState;
}

bool UdpCommunication::sendData(const QByteArray &data)
{
    if (!m_isOpen) {
        emit errorOccurred(tr("UDP 未打开，无法发送数据"), -1);
        return false;
    }

    if (data.isEmpty()) {
        emit errorOccurred(tr("发送数据为空"), -2);
        return false;
    }

    const UdpConfig &cfg = m_config.udp;
    QHostAddress targetAddress(cfg.hostAddress);

    // 发送数据报到目标地址和端口
    qint64 bytesWritten = m_udpSocket->writeDatagram(
        data, targetAddress, cfg.port);

    if (bytesWritten == -1) {
        emit errorOccurred(tr("UDP 发送数据报失败: %1")
                           .arg(m_udpSocket->errorString()),
                           static_cast<int>(m_udpSocket->error()));
        return false;
    }

    return true;
}

void UdpCommunication::onReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        // 获取数据报大小
        qint64 datagramSize = m_udpSocket->pendingDatagramSize();
        QByteArray data;
        data.resize(static_cast<int>(datagramSize));

        // 读取数据报
        qint64 bytesRead = m_udpSocket->readDatagram(data.data(), datagramSize);
        if (bytesRead > 0) {
            data.resize(static_cast<int>(bytesRead));
            emit dataReceived(data);
        }
    }
}

void UdpCommunication::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred(m_udpSocket->errorString(),
                       static_cast<int>(m_udpSocket->error()));
}