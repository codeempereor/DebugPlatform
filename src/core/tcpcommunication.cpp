#include "core/tcpcommunication.h"
#include <QDebug>

TcpCommunication::TcpCommunication(QObject *parent)
    : ICommunication(parent)
{
    // socket 在 open() 中懒创建，确保工作线程亲和性
}

TcpCommunication::~TcpCommunication()
{
    close();
    // m_tcpSocket 父对象为本类，本类析构时 Qt 自动 delete
}

bool TcpCommunication::open()
{
    if (m_isOpen) {
        return true;
    }

    const TcpConfig &cfg = m_config.tcp;

    if (cfg.hostAddress.isEmpty() || cfg.port == 0) {
        emit errorOccurred(tr("TCP 配置无效: host=%1 port=%2")
                           .arg(cfg.hostAddress).arg(cfg.port), -1);
        return false;
    }

    // 懒创建 socket（在工作线程中创建）
    if (!m_tcpSocket) {
        m_tcpSocket = new QTcpSocket(this);
        connect(m_tcpSocket, &QTcpSocket::connected,
                this, &TcpCommunication::onConnected);
        connect(m_tcpSocket, &QTcpSocket::disconnected,
                this, &TcpCommunication::onDisconnected);
        connect(m_tcpSocket, &QTcpSocket::readyRead,
                this, &TcpCommunication::onReadyRead);
        connect(m_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(onErrorOccurred(QAbstractSocket::SocketError)));
    }

    // 异步连接到主机
    m_tcpSocket->abort();
    m_tcpSocket->connectToHost(cfg.hostAddress, cfg.port);

    // 【防卡】UI 线程同步等待超时必须设置上限，避免长时间"卡死"
    const int timeout = qBound(100, cfg.connectionTimeout, 2000);

    // 等待连接（带受限超时）
    if (!m_tcpSocket->waitForConnected(timeout)) {
        const QString errStr = m_tcpSocket->errorString();
        emit errorOccurred(tr("TCP 连接失败（%1ms）: %2:%3 - %4")
                           .arg(timeout).arg(cfg.hostAddress)
                           .arg(cfg.port).arg(errStr),
                           static_cast<int>(m_tcpSocket->error()));
        m_tcpSocket->abort();
        return false;
    }

    m_isOpen = true;
    emit connected();
    return true;
}

void TcpCommunication::close()
{
    if (!m_isOpen) {
        return;
    }

    if (m_tcpSocket) {
        m_tcpSocket->disconnectFromHost();
        if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            m_tcpSocket->waitForDisconnected(500);
        }
        m_tcpSocket->abort();
    }

    m_isOpen = false;
    emit disconnected();
}

bool TcpCommunication::isOpen() const
{
    return m_isOpen
        && m_tcpSocket
        && m_tcpSocket->state() == QAbstractSocket::ConnectedState;
}

bool TcpCommunication::sendData(const QByteArray &data)
{
    if (!m_isOpen || !m_tcpSocket || m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred(tr("TCP 未连接，无法发送数据"), -1);
        return false;
    }

    if (data.isEmpty()) {
        emit errorOccurred(tr("发送数据为空"), -2);
        return false;
    }

    qint64 bytesWritten = m_tcpSocket->write(data);
    if (bytesWritten == -1) {
        emit errorOccurred(tr("TCP 写入失败: %1").arg(m_tcpSocket->errorString()),
                           static_cast<int>(m_tcpSocket->error()));
        return false;
    }

    // 确保数据发送
    m_tcpSocket->flush();
    return true;
}

void TcpCommunication::onConnected()
{
    m_isOpen = true;
    emit connected();
}

void TcpCommunication::onDisconnected()
{
    m_isOpen = false;
    emit disconnected();
}

void TcpCommunication::onReadyRead()
{
    QByteArray data = m_tcpSocket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void TcpCommunication::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred(m_tcpSocket->errorString(),
                       static_cast<int>(m_tcpSocket->error()));
}