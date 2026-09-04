#ifndef DEBUGPLATFORM_ICOMMUNICATION_H
#define DEBUGPLATFORM_ICOMMUNICATION_H

/**
 * @file icommunication.h
 * @brief 通信抽象接口基类
 *
 * 定义所有通信方式（串口、TCP、UDP）的统一接口。
 * 使用 QObject 继承，支持 Qt 信号槽机制。
 */

#include <QObject>
#include <QByteArray>
#include <QString>
#include "config.h"
#include "communication_config.h"

/**
 * @class ICommunication
 * @brief 通信接口基类
 *
 * 提供统一的 open/close/sendData 接口，以及数据接收和状态变化的信号。
 * 所有具体通信类（串口、TCP、UDP）都必须继承此类。
 */
class ICommunication : public QObject
{
    Q_OBJECT

public:
    explicit ICommunication(QObject *parent = nullptr);
    ~ICommunication() override;

    /**
     * @brief 打开通信连接
     * @return true 表示成功打开，false 表示失败
     */
    Q_INVOKABLE virtual bool open();

    /**
     * @brief 关闭通信连接
     */
    Q_INVOKABLE virtual void close();

    /**
     * @brief 检查连接是否已打开
     * @return true 表示已打开，false 表示未打开
     */
    Q_INVOKABLE virtual bool isOpen() const;

    /**
     * @brief 发送数据
     * @param data 要发送的字节数据
     * @return true 表示发送成功，false 表示失败
     */
    Q_INVOKABLE virtual bool sendData(const QByteArray &data);

    /**
     * @brief 获取当前通信配置
     * @return 通信配置的常量引用
     */
    const CommunicationConfig &config() const;

    /**
     * @brief 设置通信配置
     * @param config 新的通信配置
     */
    void setConfig(const CommunicationConfig &config);

signals:
    /**
     * @brief 连接成功信号
     *        当通信连接建立成功时发射
     */
    void connected();

    /**
     * @brief 连接断开信号
     *        当通信连接断开时发射
     */
    void disconnected();

    /**
     * @brief 数据接收信号
     *        当接收到数据时发射
     * @param data 接收到的字节数据
     */
    void dataReceived(const QByteArray &data);

    /**
     * @brief 错误发生信号
     *        当通信发生错误时发射
     * @param errorMessage 错误描述信息
     * @param errorCode 错误码
     */
    void errorOccurred(const QString &errorMessage, int errorCode);

protected:
    CommunicationConfig m_config;   ///< 通信配置
    bool m_isOpen = false;          ///< 连接状态标志
};

#endif // DEBUGPLATFORM_ICOMMUNICATION_H