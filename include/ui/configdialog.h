#ifndef DEBUGPLATFORM_CONFIGDIALOG_H
#define DEBUGPLATFORM_CONFIGDIALOG_H

/**
 * @file configdialog.h
 * @brief 设备配置管理对话框
 *
 * 展示一个设备的完整配置，并允许用户修改后保存。
 * 相比 AddDeviceDialog，本对话框面向"修改"场景，
 * 会预填现有配置并在确认时写回。
 */

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include "device/deviceconfig.h"

/**
 * @class ConfigDialog
 * @brief 设备配置编辑对话框
 */
class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param config 初始配置
     * @param parent 父对象
     */
    explicit ConfigDialog(const DeviceConfig &config, QWidget *parent = nullptr);

    /**
     * @brief 获取编辑后的配置
     */
    DeviceConfig getConfig() const;

private slots:
    void onCommTypeChanged(int index);
    void onAccepted();

private:
    void setupUi();
    void loadFromConfig(const DeviceConfig &cfg);

private:
    DeviceConfig m_originalConfig;

    QLineEdit          *m_nameEdit        = nullptr;
    QComboBox          *m_commTypeCombo  = nullptr;
    QComboBox          *m_protocolCombo  = nullptr;
    QSpinBox           *m_pollIntervalSpin = nullptr;
    QSpinBox           *m_timeoutSpin     = nullptr;
    QCheckBox          *m_autoReconnectChk = nullptr;

    QWidget            *m_serialFrame     = nullptr;
    QLineEdit          *m_serialPortEdit  = nullptr;
    QComboBox          *m_serialBaudCombo = nullptr;
    QComboBox          *m_serialDataBits  = nullptr;
    QComboBox          *m_serialStopBits  = nullptr;
    QComboBox          *m_serialParity    = nullptr;

    QWidget            *m_tcpFrame       = nullptr;
    QLineEdit          *m_tcpHostEdit    = nullptr;
    QSpinBox           *m_tcpPortSpin    = nullptr;

    QWidget            *m_udpFrame       = nullptr;
    QLineEdit          *m_udpHostEdit    = nullptr;
    QSpinBox           *m_udpPortSpin    = nullptr;
    QSpinBox           *m_udpLocalPort   = nullptr;

    QWidget            *m_mqttFrame      = nullptr;
    QLineEdit          *m_mqttBrokerHost = nullptr;
    QSpinBox           *m_mqttBrokerPort = nullptr;
    QLineEdit          *m_mqttClientId   = nullptr;
    QLineEdit          *m_mqttUsername   = nullptr;
    QLineEdit          *m_mqttPassword   = nullptr;
    QLineEdit          *m_mqttSubTopic   = nullptr;
    QLineEdit          *m_mqttPubTopic   = nullptr;
    QComboBox          *m_mqttQosCombo   = nullptr;
    QSpinBox           *m_mqttKeepAlive = nullptr;
    QCheckBox          *m_mqttCleanSession = nullptr;

    QDialogButtonBox   *m_buttonBox      = nullptr;
};

#endif // DEBUGPLATFORM_CONFIGDIALOG_H
