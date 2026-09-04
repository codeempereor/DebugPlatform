#ifndef TEST_UDPCOMMUNICATION_H
#define TEST_UDPCOMMUNICATION_H

#include <QObject>
#include <QtTest/QtTest>
#include <QHostAddress>
#include <QSignalSpy>

class QUdpSocket;
class UdpCommunication;

/**
 * @class TestUdpCommunication
 * @brief UdpCommunication 单元测试类
 *
 * 使用 QUdpSocket 作为本地回环对端，覆盖 UdpCommunication 的：
 *   - open() 成功 / 幂等 / 端口占用失败
 *   - close() 正常关闭 / 未打开关闭
 *   - isOpen() 状态查询
 *   - sendData() 成功 / 未打开 / 空数据
 *   - 数据接收路径 (onReadyRead -> dataReceived)
 *   - 错误路径 (onErrorOccurred)
 */
class TestUdpCommunication : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void cleanupTestCase();

    // open 路径
    void testOpenSuccess();
    void testOpenAlreadyOpen();
    void testOpenBindFailure();

    // close 路径
    void testCloseWhenOpen();
    void testCloseWhenNotOpen();

    // isOpen 路径
    void testIsOpenStates();

    // sendData 路径
    void testSendDataSuccess();
    void testSendDataWhenNotOpen();
    void testSendDataEmpty();

    // 数据接收路径
    void testDataReceived();

    // 信号路径
    void testConnectedSignalEmitted();
    void testDisconnectedSignalEmitted();

private:
    UdpCommunication *m_comm = nullptr;
    QUdpSocket *m_peer = nullptr;       // 用于发送/接收对端
    quint16 m_localPort = 0;           // m_comm 绑定的本地端口
};

#endif // TEST_UDPCOMMUNICATION_H
