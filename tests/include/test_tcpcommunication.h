#ifndef TEST_TCPCOMMUNICATION_H
#define TEST_TCPCOMMUNICATION_H

#include <QObject>
#include <QtTest/QtTest>
#include <QHostAddress>
#include <QSignalSpy>

class QTcpServer;
class TcpCommunication;

/**
 * @class TestTcpCommunication
 * @brief TcpCommunication 单元测试类
 *
 * 使用 QTcpServer 作为本地回环服务端，覆盖 TcpCommunication 的：
 *   - open() 成功 / 失败 / 幂等
 *   - close() 正常关闭 / 未打开关闭
 *   - isOpen() 状态查询
 *   - sendData() 成功 / 未连接 / 空数据
 *   - 数据接收路径 (onReadyRead -> dataReceived)
 *   - 错误路径 (连接不存在端口 / onErrorOccurred)
 */
class TestTcpCommunication : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void cleanupTestCase();

    // open 路径
    void testOpenSuccess();
    void testOpenAlreadyOpen();
    void testOpenInvalidConfig();
    void testOpenNoServer();

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
    void testErrorSignalEmitted();

private:
    TcpCommunication *m_comm = nullptr;
    QTcpServer *m_server = nullptr;
    quint16 m_serverPort = 0;
};

#endif // TEST_TCPCOMMUNICATION_H
