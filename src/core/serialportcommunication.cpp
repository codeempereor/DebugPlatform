#include "core/serialportcommunication.h"
#include <QDebug>

SerialPortCommunication::SerialPortCommunication(QObject *parent)
    : ICommunication(parent)
{
    // socket 不在此创建；移到 open() 里懒创建，保证：
    // 1) 父对象为本类，随 moveToThread 一起迁移到工作线程；
    // 2) socket 始终在工作线程的事件循环中被创建与使用，避免跨线程访问。
}

SerialPortCommunication::~SerialPortCommunication()
{
    close();
    // m_serialPort 父对象为本类，本类析构时 Qt 自动 delete 子对象
}

bool SerialPortCommunication::open()
{
    if (m_isOpen) {
        return true;
    }

    const SerialConfig &cfg = m_config.serial;

    // 懒创建 socket（在工作线程中创建，父对象为本类）
    if (!m_serialPort) {
        m_serialPort = new QSerialPort(this);
        connect(m_serialPort, &QSerialPort::readyRead,
                this, &SerialPortCommunication::onReadyRead);
        connect(m_serialPort, &QSerialPort::errorOccurred,
                this, &SerialPortCommunication::onErrorOccurred);
    }

    // 配置串口参数
    m_serialPort->setPortName(cfg.portName);
    m_serialPort->setBaudRate(cfg.baudRate);
    m_serialPort->setDataBits(cfg.dataBits);
    m_serialPort->setStopBits(cfg.stopBits);
    m_serialPort->setParity(cfg.parity);
    m_serialPort->setFlowControl(cfg.flowControl);

    // 打开串口
    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        emit errorOccurred(tr("串口打开失败: %1").arg(m_serialPort->errorString()),
                           static_cast<int>(m_serialPort->error()));
        return false;
    }

    m_isOpen = true;
    emit connected();
    return true;
}

void SerialPortCommunication::close()
{
    if (!m_isOpen) {
        return;
    }

    if (m_serialPort) {
        m_serialPort->close();
    }
    m_isOpen = false;
    emit disconnected();
}

bool SerialPortCommunication::isOpen() const
{
    return m_isOpen && m_serialPort && m_serialPort->isOpen();
}

bool SerialPortCommunication::sendData(const QByteArray &data)
{
    if (!m_isOpen || !m_serialPort || !m_serialPort->isOpen()) {
        emit errorOccurred(tr("串口未打开，无法发送数据"), -1);
        return false;
    }

    if (data.isEmpty()) {
        emit errorOccurred(tr("发送数据为空"), -2);
        return false;
    }

    qint64 bytesWritten = m_serialPort->write(data);
    if (bytesWritten == -1) {
        emit errorOccurred(tr("串口写入失败: %1").arg(m_serialPort->errorString()),
                           static_cast<int>(m_serialPort->error()));
        return false;
    }

    // 确保数据写入完成
    m_serialPort->flush();
    return true;
}

void SerialPortCommunication::onReadyRead()
{
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void SerialPortCommunication::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    emit errorOccurred(m_serialPort->errorString(),
                       static_cast<int>(error));
}