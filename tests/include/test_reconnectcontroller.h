#ifndef TEST_RECONNECTCONTROLLER_H
#define TEST_RECONNECTCONTROLLER_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTimer>
#include "device/reconnectcontroller.h"
#include "core/icommunication.h"

class MockCommunication : public ICommunication
{
    Q_OBJECT
public:
    explicit MockCommunication(QObject *parent = nullptr) : ICommunication(parent) {}
    
    void setOpenResult(bool result) { m_openResult = result; }
    bool lastOpenCalled() const { return m_lastOpenCalled; }
    void resetOpenCalled() { m_lastOpenCalled = false; }
    
    bool open() override {
        m_lastOpenCalled = true;
        if (m_openResult) {
            m_isOpen = true;
            emit connected();
        }
        return m_openResult;
    }
    
    void close() override {
        m_isOpen = false;
        emit disconnected();
    }
    
    bool sendData(const QByteArray &) override { return m_isOpen; }
    
private:
    bool m_openResult = false;
    bool m_lastOpenCalled = false;
};

class TestReconnectController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testInitialState();
    void testStartSuccessful();
    void testStartFailedThenSucceed();
    void testStop();
    void testOnDisconnected();
    void testExponentialBackoff();
    void testStateTransitions();

    // P1-3a 错误路径测试
    void testMaxAttemptsReached();
    void testReconnectAfterDisconnection();
    void testStopCancelsPendingReconnect();
    
private:
    MockCommunication *m_mockComm;
    ReconnectController *m_controller;
    DeviceConfig m_config;
};

#endif
