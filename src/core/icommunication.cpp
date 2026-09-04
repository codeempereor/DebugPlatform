#include "core/icommunication.h"

ICommunication::ICommunication(QObject *parent)
    : QObject(parent)
{
}

ICommunication::~ICommunication()
{
    if (m_isOpen) {
        close();
    }
}

bool ICommunication::open()
{
    if (m_isOpen) {
        return true; // 已经打开
    }

    // 基类只做状态检查，具体的打开逻辑由子类实现
    return false;
}

void ICommunication::close()
{
    if (!m_isOpen) {
        return; // 已经关闭
    }

    m_isOpen = false;
    emit disconnected();
}

bool ICommunication::isOpen() const
{
    return m_isOpen;
}

bool ICommunication::sendData(const QByteArray &data)
{
    if (!m_isOpen) {
        emit errorOccurred(tr("通信未打开，无法发送数据"), -1);
        return false;
    }

    if (data.isEmpty()) {
        emit errorOccurred(tr("发送数据为空"), -2);
        return false;
    }

    // 基类只做基础检查，实际发送由子类实现
    return false;
}

const CommunicationConfig &ICommunication::config() const
{
    return m_config;
}

void ICommunication::setConfig(const CommunicationConfig &config)
{
    m_config = config;
}