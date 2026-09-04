#ifndef DEBUGPLATFORM_ADDDEVICEDIALOG_H
#define DEBUGPLATFORM_ADDDEVICEDIALOG_H

/**
 * @file adddevicedialog.h
 * @brief 添加设备对话框
 *
 * 通过表单采集新设备的完整配置信息，包括：
 *   - 设备名称、通信类型、协议类型
 *   - 按通信类型动态显示的串口/TCP/UDP 配置表单
 *   - 轮询间隔、超时时间
 * 使用 QDialogButtonBox 进行确认/取消。
 */

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QWidget>
#include "core/config.h"
#include "device/deviceconfig.h"

/**
 * @class AddDeviceDialog
 * @brief 添加设备对话框
 */
class AddDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit AddDeviceDialog(QWidget *parent = nullptr);

    /**
     * @brief 根据当前表单构建 DeviceConfig
     */
    DeviceConfig getConfig() const;

private slots:
    void onCommTypeChanged(int index);
    void onProtocolTypeChanged(int index);
    void onAccepted();

private:
    /**
     * @brief 根据通信类型显示/隐藏对应表单
     */
    void updateCommConfigVisibility();

private:
    // 基础
    QLineEdit          *m_nameEdit        = nullptr;
    QComboBox          *m_commTypeCombo  = nullptr;
    QComboBox          *m_protocolCombo  = nullptr;

    // 公共
    QSpinBox           *m_pollIntervalSpin = nullptr;
    QSpinBox           *m_timeoutSpin     = nullptr;
    QCheckBox          *m_autoReconnectCheck = nullptr;

    // 串口表单
    QWidget            *m_serialFrame     = nullptr;
    QLineEdit          *m_serialPortEdit  = nullptr;
    QComboBox          *m_serialBaudCombo = nullptr;
    QComboBox          *m_serialDataBits = nullptr;
    QComboBox          *m_serialStopBits = nullptr;
    QComboBox          *m_serialParity    = nullptr;

    // TCP 表单
    QWidget            *m_tcpFrame       = nullptr;
    QLineEdit          *m_tcpHostEdit    = nullptr;
    QSpinBox           *m_tcpPortSpin    = nullptr;

    // UDP 表单
    QWidget            *m_udpFrame       = nullptr;
    QLineEdit          *m_udpHostEdit   = nullptr;
    QSpinBox           *m_udpPortSpin   = nullptr;
    QSpinBox           *m_udpLocalPort  = nullptr;

    // MQTT 表单
    QWidget            *m_mqttFrame         = nullptr;
    QLineEdit          *m_mqttBrokerHost    = nullptr;
    QSpinBox           *m_mqttBrokerPort    = nullptr;
    QLineEdit          *m_mqttClientId      = nullptr;
    QLineEdit          *m_mqttUsername      = nullptr;
    QLineEdit          *m_mqttPassword      = nullptr;
    QLineEdit          *m_mqttSubTopic      = nullptr;
    QLineEdit          *m_mqttPubTopic      = nullptr;
    QComboBox          *m_mqttQosCombo      = nullptr;
    QSpinBox           *m_mqttKeepAlive     = nullptr;
    QCheckBox          *m_mqttCleanSession  = nullptr;

    QDialogButtonBox   *m_buttonBox     = nullptr;
};

#endif // DEBUGPLATFORM_ADDDEVICEDIALOG_H
