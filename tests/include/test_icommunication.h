#ifndef TEST_ICOMMUNICATION_H
#define TEST_ICOMMUNICATION_H

#include <QObject>
#include <QtTest/QtTest>
#include <QSignalSpy>
#include "core/icommunication.h"

class TestCommunication : public ICommunication
{
    Q_OBJECT
public:
    explicit TestCommunication(QObject *parent = nullptr) : ICommunication(parent) {}
    
    bool open() override {
        if (m_openResult) {
            m_isOpen = true;
            emit connected();
        }
        return m_openResult;
    }
    
    void close() override {
        if (m_isOpen) {
            m_isOpen = false;
            emit disconnected();
        }
    }
    
    bool sendData(const QByteArray &data) override {
        if (!m_isOpen) return false;
        m_sentData = data;
        return true;
    }
    
    void simulateReceive(const QByteArray &data) {
        emit dataReceived(data);
    }
    
    void simulateError(const QString &msg, int code) {
        emit errorOccurred(msg, code);
    }
    
    void setOpenResult(bool result) { m_openResult = result; }
    QByteArray sentData() const { return m_sentData; }
    
private:
    bool m_openResult = true;
    QByteArray m_sentData;
};

class TestICommunication : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testConfig();
    void testSetConfig();
    void testOpenClose();
    void testSendData();
    void testSignals();
    void testNotOpen();
    
private:
    TestCommunication *m_comm;
};

#endif
