#ifndef DEBUGPLATFORM_SERIALPORTCOMMUNICATION_H
#define DEBUGPLATFORM_SERIALPORTCOMMUNICATION_H

/**
 * @file serialportcommunication.h
 * @brief 串口通信实现类
 *
 * 基于 QSerialPort 实现串口通信功能，支持配置波特率、数据位、停止位、校验位和流控。
 */

#include "core/icommunication.h"
#include <QSerialPort>

/**
 * @class SerialPortCommunication
 * @brief 串口通信类
 *
 * 封装 QSerialPort，实现 ICommunication 接口，提供串口打开、关闭、数据收发等功能。
 */
class SerialPortCommunication : public ICommunication
{
    Q_OBJECT

public:
    explicit SerialPortCommunication(QObject *parent = nullptr);
    ~SerialPortCommunication() override;

    bool open() override;
    void close() override;
    bool isOpen() const override;
    bool sendData(const QByteArray &data) override;

private slots:
    /**
     * @brief 处理 QSerialPort 的 readyRead 信号
     *        读取所有可用数据并发射 dataReceived 信号
     */
    void onReadyRead();

    /**
     * @brief 处理 QSerialPort 的 errorOccurred 信号
     * @param error 串口错误类型
     */
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serialPort = nullptr;  ///< 串口对象（懒创建于 open()，父对象为本类，随 moveToThread 一起迁移）
};

#endif // DEBUGPLATFORM_SERIALPORTCOMMUNICATION_H