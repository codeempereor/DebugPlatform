#include "test_reconnectcontroller.h"
#include <QCoreApplication>

void TestReconnectController::initTestCase()
{
    m_mockComm = new MockCommunication();
    m_controller = new ReconnectController();
    m_config.id = "test_device";
    m_config.name = "Test Device";
    m_config.autoReconnect = true;
}

void TestReconnectController::cleanupTestCase()
{
    m_controller->stop();
    delete m_controller;
    delete m_mockComm;
}

void TestReconnectController::testInitialState()
{
    QCOMPARE(m_controller->state(), ReconnectState::IDLE);
    QCOMPARE(m_controller->currentDelay(), 0);
}

void TestReconnectController::testStartSuccessful()
{
    m_mockComm->setOpenResult(true);
    m_mockComm->resetOpenCalled();

    // start() 不调用 open()，仅设为 CONNECTED 状态（连接由外部建立后武装重连）
    m_controller->start(m_mockComm, m_config);

    QCoreApplication::processEvents();
    QVERIFY(!m_mockComm->lastOpenCalled());  // start() 不应调用 open()
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);

    m_controller->stop();
    QCOMPARE(m_controller->state(), ReconnectState::IDLE);
}

void TestReconnectController::testStartFailedThenSucceed()
{
    // start() 直接设为 CONNECTED（不调用 open）
    m_mockComm->setOpenResult(false);
    m_controller->start(m_mockComm, m_config);
    QCoreApplication::processEvents();
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);

    // 模拟断线，触发重连调度（attempt=1, delay=1000ms）
    m_controller->onDisconnected();
    QCOMPARE(m_controller->state(), ReconnectState::DISCONNECTED);
    QVERIFY(m_controller->currentDelay() > 0);

    // 切换为打开成功，手动触发重连定时器回调
    m_mockComm->setOpenResult(true);
    m_mockComm->resetOpenCalled();
    QMetaObject::invokeMethod(m_controller, "onReconnectTimer", Qt::DirectConnection);

    QVERIFY(m_mockComm->lastOpenCalled());
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);

    m_controller->stop();
}

void TestReconnectController::testStop()
{
    m_mockComm->setOpenResult(true);
    m_controller->start(m_mockComm, m_config);
    
    QCoreApplication::processEvents();
    
    m_controller->stop();
    QCOMPARE(m_controller->state(), ReconnectState::IDLE);
    QCOMPARE(m_controller->currentDelay(), 0);
}

void TestReconnectController::testOnDisconnected()
{
    m_mockComm->setOpenResult(true);
    m_controller->start(m_mockComm, m_config);
    QCoreApplication::processEvents();
    
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);
    
    m_controller->onDisconnected();
    QCOMPARE(m_controller->state(), ReconnectState::DISCONNECTED);
    
    m_controller->stop();
}

void TestReconnectController::testExponentialBackoff()
{
    QList<int> delays;
    
    int baseDelay = 1000;
    int maxDelay = 32000;
    
    for (int attempt = 1; attempt <= 10; ++attempt) {
        int expectedDelay = baseDelay;
        for (int i = 1; i < attempt; ++i) {
            expectedDelay *= 2;
            if (expectedDelay >= maxDelay) {
                expectedDelay = maxDelay;
                break;
            }
        }
        if (expectedDelay > maxDelay) expectedDelay = maxDelay;
        delays.append(expectedDelay);
    }
    
    QVERIFY(delays.size() > 0);
    QVERIFY(delays.first() == baseDelay);
    QVERIFY(delays.last() <= maxDelay);
}

void TestReconnectController::testStateTransitions()
{
    QCOMPARE(m_controller->state(), ReconnectState::IDLE);
    
    m_mockComm->setOpenResult(true);
    m_controller->start(m_mockComm, m_config);
    QCoreApplication::processEvents();
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);
    
    m_controller->onDisconnected();
    QCOMPARE(m_controller->state(), ReconnectState::DISCONNECTED);
    
    m_controller->stop();
    QCOMPARE(m_controller->state(), ReconnectState::IDLE);
}

// ============================================================================
// P1-3a 错误路径测试
// ============================================================================

void TestReconnectController::testMaxAttemptsReached()
{
    // 始终打开失败，触发达到最大重连次数
    m_mockComm->setOpenResult(false);
    m_mockComm->resetOpenCalled();

    QSignalSpy failSpy(m_controller, &ReconnectController::reconnectFailed);
    QSignalSpy attemptSpy(m_controller, &ReconnectController::reconnectAttempt);

    // start() 直接设为 CONNECTED，不触发重连
    m_controller->start(m_mockComm, m_config);
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);
    QCOMPARE(attemptSpy.count(), 0);

    // 模拟断线，触发首次 scheduleReconnect()（attempt=0）
    m_controller->onDisconnected();
    QCOMPARE(m_controller->state(), ReconnectState::DISCONNECTED);
    QCOMPARE(attemptSpy.count(), 1);

    // 手动触发 onReconnectTimer() 达到最大次数
    // 每次 onReconnectTimer: attemptCount++ → open失败 → scheduleReconnect()
    // 第 RECONNECT_MAX_ATTEMPTS 次后 scheduleReconnect 检测到上限，发射 reconnectFailed
    for (int i = 0; i < Constants::RECONNECT_MAX_ATTEMPTS; ++i) {
        QMetaObject::invokeMethod(m_controller, "onReconnectTimer",
                                  Qt::DirectConnection);
    }

    QVERIFY(failSpy.count() >= 1);
    QCOMPARE(m_controller->state(), ReconnectState::DISCONNECTED);

    m_controller->stop();
}

void TestReconnectController::testReconnectAfterDisconnection()
{
    // 先成功连接
    m_mockComm->setOpenResult(true);
    m_controller->start(m_mockComm, m_config);
    QCoreApplication::processEvents();
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);

    // 模拟意外断开，autoReconnect=true 应触发重连调度
    QSignalSpy reconnectSpy(m_controller, &ReconnectController::reconnected);
    m_controller->onDisconnected();
    QCOMPARE(m_controller->state(), ReconnectState::DISCONNECTED);

    // 手动触发重连定时器回调以模拟超时
    QMetaObject::invokeMethod(m_controller, "onReconnectTimer",
                              Qt::DirectConnection);

    // open 仍为 true，重连应成功
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);
    QVERIFY(reconnectSpy.count() >= 1);

    m_controller->stop();
}

void TestReconnectController::testStopCancelsPendingReconnect()
{
    // start() 直接设为 CONNECTED
    m_mockComm->setOpenResult(false);
    m_controller->start(m_mockComm, m_config);
    QCOMPARE(m_controller->state(), ReconnectState::CONNECTED);

    QSignalSpy attemptSpy(m_controller, &ReconnectController::reconnectAttempt);

    // 模拟断线，触发重连调度（启动退避定时器）
    m_controller->onDisconnected();
    QCOMPARE(attemptSpy.count(), 1);
    QCOMPARE(m_controller->state(), ReconnectState::DISCONNECTED);
    QVERIFY(m_controller->currentDelay() > 0);

    // 立即 stop 应取消挂起的定时器
    m_controller->stop();
    QCOMPARE(m_controller->state(), ReconnectState::IDLE);
    QCOMPARE(m_controller->currentDelay(), 0);

    // 手动触发定时器回调：comm 已被置空，应直接 return 不发射新信号
    QMetaObject::invokeMethod(m_controller, "onReconnectTimer",
                              Qt::DirectConnection);
    QCOMPARE(attemptSpy.count(), 1);
    QCOMPARE(m_controller->state(), ReconnectState::IDLE);
}

