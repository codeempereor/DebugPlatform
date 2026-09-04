#ifndef DEBUGPLATFORM_UDPCOMMUNICATION_H
#define DEBUGPLATFORM_UDPCOMMUNICATION_H

/**
 * @file udpcommunication.h
 * @brief UDP 客户端通信实现类
 *
 * 基于 QUdpSocket 实现 UDP 通信，支持数据报的发送和接收。
 */

#include "core/icommunication.h"
#include <QUdpSocket>

/**
 * @class UdpCommunication
 * @brief UDP 通信类
 *
 * 封装 QUdpSocket，实现 ICommunication 接口，提供 UDP 数据报
 * 的发送和接收功能。支持绑定本地端口。
 */
class UdpCommunication : public ICommunication
{
    Q_OBJECT

public:
    explicit UdpCommunication(QObject *parent = nullptr);
    ~UdpCommunication() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    bool sendData(const QByteArray &data) override;

private slots:
    /**
     * @brief 处理 QUdpSocket 的 readyRead 信号
     *        读取所有可用数据报并发射 dataReceived 信号
     */
    void onReadyRead();

    /**
     * @brief 处理 QUdpSocket 的 errorOccurred 信号
     * @param socketError Socket 错误类型
     */
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QUdpSocket *m_udpSocket = nullptr;  ///< UDP Socket（懒创建于 open()，父对象为本类）
};

#endif // DEBUGPLATFORM_UDPCOMMUNICATION_H