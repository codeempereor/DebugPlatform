#ifndef DEBUGPLATFORM_TCPCOMMUNICATION_H
#define DEBUGPLATFORM_TCPCOMMUNICATION_H

/**
 * @file tcpcommunication.h
 * @brief TCP 客户端通信实现类
 *
 * 基于 QTcpSocket 实现 TCP 客户端通信，支持异步连接和数据收发。
 */

#include "core/icommunication.h"
#include <QTcpSocket>

/**
 * @class TcpCommunication
 * @brief TCP 客户端通信类
 *
 * 封装 QTcpSocket，实现 ICommunication 接口，提供 TCP 异步连接、
 * 数据收发等功能。支持连接超时配置。
 */
class TcpCommunication : public ICommunication
{
    Q_OBJECT

public:
    explicit TcpCommunication(QObject *parent = nullptr);
    ~TcpCommunication() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    bool sendData(const QByteArray &data) override;

private slots:
    /**
     * @brief 处理 QTcpSocket 的 connected 信号
     */
    void onConnected();

    /**
     * @brief 处理 QTcpSocket 的 disconnected 信号
     */
    void onDisconnected();

    /**
     * @brief 处理 QTcpSocket 的 readyRead 信号
     *        读取所有可用数据并发射 dataReceived 信号
     */
    void onReadyRead();

    /**
     * @brief 处理 QTcpSocket 的 errorOccurred 信号
     * @param socketError Socket 错误类型
     */
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_tcpSocket = nullptr;  ///< TCP Socket（懒创建于 open()，父对象为本类）
};

#endif // DEBUGPLATFORM_TCPCOMMUNICATION_H