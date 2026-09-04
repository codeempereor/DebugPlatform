#include "ui/configdialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QCheckBox>
#include <QSerialPort>

ConfigDialog::ConfigDialog(const DeviceConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_originalConfig(config)
{
    setWindowTitle(tr("设备配置 - %1").arg(config.name));
    resize(460, 520);
    setupUi();
    loadFromConfig(config);
}

// ============================================================================
// 公共 API
// ============================================================================

DeviceConfig ConfigDialog::getConfig() const
{
    DeviceConfig cfg = m_originalConfig;
    cfg.name = m_nameEdit->text().trimmed();
    cfg.commType = static_cast<CommunicationType>(
        m_commTypeCombo->currentData().toInt());
    cfg.protocolType = static_cast<ProtocolType>(
        m_protocolCombo->currentData().toInt());
    cfg.pollInterval    = m_pollIntervalSpin->value();
    cfg.timeout         = m_timeoutSpin->value();
    cfg.autoReconnect   = m_autoReconnectChk->isChecked();

    switch (cfg.commType) {
    case CommunicationType::Serial:
        cfg.commConfig.type = CommunicationType::Serial;
        cfg.commConfig.serial.portName = m_serialPortEdit->text().trimmed();
        cfg.commConfig.serial.baudRate = m_serialBaudCombo->currentData().toInt();
        cfg.commConfig.serial.dataBits = static_cast<QSerialPort::DataBits>(
            m_serialDataBits->currentData().toInt());
        cfg.commConfig.serial.stopBits = static_cast<QSerialPort::StopBits>(
            m_serialStopBits->currentData().toInt());
        cfg.commConfig.serial.parity   = static_cast<QSerialPort::Parity>(
            m_serialParity->currentData().toInt());
        cfg.commConfig.serial.flowControl = QSerialPort::NoFlowControl;
        break;
    case CommunicationType::Tcp:
        cfg.commConfig.type = CommunicationType::Tcp;
        cfg.commConfig.tcp.hostAddress = m_tcpHostEdit->text().trimmed();
        cfg.commConfig.tcp.port        = static_cast<quint16>(m_tcpPortSpin->value());
        break;
    case CommunicationType::Udp:
        cfg.commConfig.type = CommunicationType::Udp;
        cfg.commConfig.udp.hostAddress = m_udpHostEdit->text().trimmed();
        cfg.commConfig.udp.port        = static_cast<quint16>(m_udpPortSpin->value());
        cfg.commConfig.udp.localPort   = static_cast<quint16>(m_udpLocalPort->value());
        break;
    case CommunicationType::Mqtt:
        cfg.commConfig.type = CommunicationType::Mqtt;
        cfg.commConfig.mqtt.brokerHost   = m_mqttBrokerHost->text().trimmed();
        cfg.commConfig.mqtt.brokerPort   = static_cast<quint16>(m_mqttBrokerPort->value());
        cfg.commConfig.mqtt.clientId     = m_mqttClientId->text().trimmed();
        cfg.commConfig.mqtt.username     = m_mqttUsername->text().trimmed();
        cfg.commConfig.mqtt.password     = m_mqttPassword->text();
        cfg.commConfig.mqtt.subscribeTopic = m_mqttSubTopic->text().trimmed();
        cfg.commConfig.mqtt.publishTopic   = m_mqttPubTopic->text().trimmed();
        cfg.commConfig.mqtt.qos          = m_mqttQosCombo->currentData().toInt();
        cfg.commConfig.mqtt.keepAlive    = m_mqttKeepAlive->value();
        cfg.commConfig.mqtt.cleanSession = m_mqttCleanSession->isChecked();
        break;
    }
    return cfg;
}

// ============================================================================
// 内部实现
// ============================================================================

void ConfigDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // 基础
    auto *baseGroup = new QGroupBox(tr("基础信息"), this);
    auto *baseForm = new QFormLayout(baseGroup);
    m_nameEdit = new QLineEdit(baseGroup);
    baseForm->addRow(tr("设备名称："), m_nameEdit);

    m_commTypeCombo = new QComboBox(baseGroup);
    m_commTypeCombo->addItem(tr("串口 (Serial)"), static_cast<int>(CommunicationType::Serial));
    m_commTypeCombo->addItem(tr("TCP"),            static_cast<int>(CommunicationType::Tcp));
    m_commTypeCombo->addItem(tr("UDP"),            static_cast<int>(CommunicationType::Udp));
    m_commTypeCombo->addItem(tr("MQTT"),           static_cast<int>(CommunicationType::Mqtt));
    baseForm->addRow(tr("通信类型："), m_commTypeCombo);

    m_protocolCombo = new QComboBox(baseGroup);
    m_protocolCombo->addItem(tr("BinaryFrame"), static_cast<int>(ProtocolType::BinaryFrame));
    m_protocolCombo->addItem(tr("Modbus RTU"),  static_cast<int>(ProtocolType::ModbusRtu));
    m_protocolCombo->addItem(tr("Modbus TCP"),  static_cast<int>(ProtocolType::ModbusTcp));
    m_protocolCombo->addItem(tr("MqttJson"),   static_cast<int>(ProtocolType::MqttJson));
    baseForm->addRow(tr("协议类型："), m_protocolCombo);

    mainLayout->addWidget(baseGroup);

    // 通信配置
    auto *commGroup = new QGroupBox(tr("通信配置"), this);
    auto *commLayout = new QVBoxLayout(commGroup);

    // 串口
    m_serialFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_serialFrame);
        form->setContentsMargins(4, 4, 4, 4);

        m_serialPortEdit = new QLineEdit(m_serialFrame);
        form->addRow(tr("端口："), m_serialPortEdit);

        m_serialBaudCombo = new QComboBox(m_serialFrame);
        for (qint32 b : {9600, 19200, 38400, 57600, 115200, 230400, 460800})
            m_serialBaudCombo->addItem(QString::number(b), b);
        form->addRow(tr("波特率："), m_serialBaudCombo);

        m_serialDataBits = new QComboBox(m_serialFrame);
        m_serialDataBits->addItem(QStringLiteral("5"), static_cast<int>(QSerialPort::Data5));
        m_serialDataBits->addItem(QStringLiteral("6"), static_cast<int>(QSerialPort::Data6));
        m_serialDataBits->addItem(QStringLiteral("7"), static_cast<int>(QSerialPort::Data7));
        m_serialDataBits->addItem(QStringLiteral("8"), static_cast<int>(QSerialPort::Data8));
        form->addRow(tr("数据位："), m_serialDataBits);

        m_serialStopBits = new QComboBox(m_serialFrame);
        m_serialStopBits->addItem(QStringLiteral("1"),   static_cast<int>(QSerialPort::OneStop));
        m_serialStopBits->addItem(QStringLiteral("1.5"), static_cast<int>(QSerialPort::OneAndHalfStop));
        m_serialStopBits->addItem(QStringLiteral("2"),   static_cast<int>(QSerialPort::TwoStop));
        form->addRow(tr("停止位："), m_serialStopBits);

        m_serialParity = new QComboBox(m_serialFrame);
        m_serialParity->addItem(tr("None"),  static_cast<int>(QSerialPort::NoParity));
        m_serialParity->addItem(tr("Even"),  static_cast<int>(QSerialPort::EvenParity));
        m_serialParity->addItem(tr("Odd"),   static_cast<int>(QSerialPort::OddParity));
        m_serialParity->addItem(tr("Space"), static_cast<int>(QSerialPort::SpaceParity));
        m_serialParity->addItem(tr("Mark"),  static_cast<int>(QSerialPort::MarkParity));
        form->addRow(tr("校验位："), m_serialParity);
    }

    // TCP
    m_tcpFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_tcpFrame);
        form->setContentsMargins(4, 4, 4, 4);
        m_tcpHostEdit = new QLineEdit(m_tcpFrame);
        m_tcpHostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
        form->addRow(tr("主机地址："), m_tcpHostEdit);
        m_tcpPortSpin = new QSpinBox(m_tcpFrame);
        m_tcpPortSpin->setRange(1, 65535);
        form->addRow(tr("端口："), m_tcpPortSpin);
    }

    // UDP
    m_udpFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_udpFrame);
        form->setContentsMargins(4, 4, 4, 4);
        m_udpHostEdit = new QLineEdit(m_udpFrame);
        m_udpHostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
        form->addRow(tr("主机地址："), m_udpHostEdit);
        m_udpPortSpin = new QSpinBox(m_udpFrame);
        m_udpPortSpin->setRange(1, 65535);
        form->addRow(tr("端口："), m_udpPortSpin);
        m_udpLocalPort = new QSpinBox(m_udpFrame);
        m_udpLocalPort->setRange(0, 65535);
        m_udpLocalPort->setSpecialValueText(tr("自动分配"));
        form->addRow(tr("本地端口："), m_udpLocalPort);
    }

    // MQTT
    m_mqttFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_mqttFrame);
        form->setContentsMargins(4, 4, 4, 4);

        m_mqttBrokerHost = new QLineEdit(m_mqttFrame);
        m_mqttBrokerHost->setPlaceholderText(QStringLiteral("broker.emqx.io"));
        form->addRow(tr("Broker 地址："), m_mqttBrokerHost);

        m_mqttBrokerPort = new QSpinBox(m_mqttFrame);
        m_mqttBrokerPort->setRange(1, 65535);
        m_mqttBrokerPort->setValue(1883);
        form->addRow(tr("Broker 端口："), m_mqttBrokerPort);

        m_mqttClientId = new QLineEdit(m_mqttFrame);
        m_mqttClientId->setPlaceholderText(tr("留空自动生成"));
        form->addRow(tr("Client ID："), m_mqttClientId);

        m_mqttUsername = new QLineEdit(m_mqttFrame);
        m_mqttUsername->setPlaceholderText(tr("可选"));
        form->addRow(tr("用户名："), m_mqttUsername);

        m_mqttPassword = new QLineEdit(m_mqttFrame);
        m_mqttPassword->setEchoMode(QLineEdit::Password);
        m_mqttPassword->setPlaceholderText(tr("可选"));
        form->addRow(tr("密码："), m_mqttPassword);

        m_mqttSubTopic = new QLineEdit(m_mqttFrame);
        m_mqttSubTopic->setPlaceholderText(QStringLiteral("devices/+/data/#"));
        form->addRow(tr("订阅主题："), m_mqttSubTopic);

        m_mqttPubTopic = new QLineEdit(m_mqttFrame);
        m_mqttPubTopic->setPlaceholderText(QStringLiteral("devices/+/reply"));
        form->addRow(tr("发布主题："), m_mqttPubTopic);

        m_mqttQosCombo = new QComboBox(m_mqttFrame);
        m_mqttQosCombo->addItem(QStringLiteral("0 - At Most Once"), 0);
        m_mqttQosCombo->addItem(QStringLiteral("1 - At Least Once"), 1);
        m_mqttQosCombo->addItem(QStringLiteral("2 - Exactly Once"), 2);
        form->addRow(tr("QoS："), m_mqttQosCombo);

        m_mqttKeepAlive = new QSpinBox(m_mqttFrame);
        m_mqttKeepAlive->setRange(10, 65535);
        m_mqttKeepAlive->setValue(60);
        m_mqttKeepAlive->setSuffix(QStringLiteral(" s"));
        form->addRow(tr("Keep Alive："), m_mqttKeepAlive);

        m_mqttCleanSession = new QCheckBox(tr("Clean Session"), m_mqttFrame);
        m_mqttCleanSession->setChecked(true);
        form->addRow(m_mqttCleanSession);
    }

    commLayout->addWidget(m_serialFrame);
    commLayout->addWidget(m_tcpFrame);
    commLayout->addWidget(m_udpFrame);
    commLayout->addWidget(m_mqttFrame);
    mainLayout->addWidget(commGroup);

    // 轮询/超时
    auto *pollGroup = new QGroupBox(tr("轮询与重连"), this);
    auto *pollForm = new QFormLayout(pollGroup);

    m_pollIntervalSpin = new QSpinBox(pollGroup);
    m_pollIntervalSpin->setRange(1, 60000);
    m_pollIntervalSpin->setSuffix(QStringLiteral(" ms"));
    pollForm->addRow(tr("轮询间隔："), m_pollIntervalSpin);

    m_timeoutSpin = new QSpinBox(pollGroup);
    m_timeoutSpin->setRange(1, 60000);
    m_timeoutSpin->setSuffix(QStringLiteral(" ms"));
    pollForm->addRow(tr("超时："), m_timeoutSpin);

    m_autoReconnectChk = new QCheckBox(tr("启用自动重连"), pollGroup);
    pollForm->addRow(m_autoReconnectChk);

    mainLayout->addWidget(pollGroup);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_commTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigDialog::onCommTypeChanged);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &ConfigDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

void ConfigDialog::loadFromConfig(const DeviceConfig &cfg)
{
    m_nameEdit->setText(cfg.name);
    m_commTypeCombo->setCurrentIndex(
        m_commTypeCombo->findData(static_cast<int>(cfg.commType)));
    m_protocolCombo->setCurrentIndex(
        m_protocolCombo->findData(static_cast<int>(cfg.protocolType)));
    m_pollIntervalSpin->setValue(cfg.pollInterval);
    m_timeoutSpin->setValue(cfg.timeout);
    m_autoReconnectChk->setChecked(cfg.autoReconnect);

    switch (cfg.commType) {
    case CommunicationType::Serial:
        m_serialPortEdit->setText(cfg.commConfig.serial.portName);
        m_serialBaudCombo->setCurrentIndex(
            m_serialBaudCombo->findData(cfg.commConfig.serial.baudRate));
        m_serialDataBits->setCurrentIndex(
            m_serialDataBits->findData(static_cast<int>(cfg.commConfig.serial.dataBits)));
        m_serialStopBits->setCurrentIndex(
            m_serialStopBits->findData(static_cast<int>(cfg.commConfig.serial.stopBits)));
        m_serialParity->setCurrentIndex(
            m_serialParity->findData(static_cast<int>(cfg.commConfig.serial.parity)));
        break;
    case CommunicationType::Tcp:
        m_tcpHostEdit->setText(cfg.commConfig.tcp.hostAddress);
        m_tcpPortSpin->setValue(cfg.commConfig.tcp.port);
        break;
    case CommunicationType::Udp:
        m_udpHostEdit->setText(cfg.commConfig.udp.hostAddress);
        m_udpPortSpin->setValue(cfg.commConfig.udp.port);
        m_udpLocalPort->setValue(cfg.commConfig.udp.localPort);
        break;
    case CommunicationType::Mqtt:
        m_mqttBrokerHost->setText(cfg.commConfig.mqtt.brokerHost);
        m_mqttBrokerPort->setValue(cfg.commConfig.mqtt.brokerPort);
        m_mqttClientId->setText(cfg.commConfig.mqtt.clientId);
        m_mqttUsername->setText(cfg.commConfig.mqtt.username);
        m_mqttPassword->setText(cfg.commConfig.mqtt.password);
        m_mqttSubTopic->setText(cfg.commConfig.mqtt.subscribeTopic);
        m_mqttPubTopic->setText(cfg.commConfig.mqtt.publishTopic);
        m_mqttQosCombo->setCurrentIndex(
            m_mqttQosCombo->findData(cfg.commConfig.mqtt.qos));
        m_mqttKeepAlive->setValue(cfg.commConfig.mqtt.keepAlive);
        m_mqttCleanSession->setChecked(cfg.commConfig.mqtt.cleanSession);
        break;
    }

    onCommTypeChanged(m_commTypeCombo->currentIndex());
}

void ConfigDialog::onCommTypeChanged(int index)
{
    Q_UNUSED(index);
    const auto t = static_cast<CommunicationType>(
        m_commTypeCombo->currentData().toInt());
    m_serialFrame->setVisible(t == CommunicationType::Serial);
    m_tcpFrame->setVisible(t == CommunicationType::Tcp);
    m_udpFrame->setVisible(t == CommunicationType::Udp);
    m_mqttFrame->setVisible(t == CommunicationType::Mqtt);
}

void ConfigDialog::onAccepted()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("校验失败"), tr("设备名称不能为空。"));
        return;
    }
    const auto t = static_cast<CommunicationType>(
        m_commTypeCombo->currentData().toInt());
    if (t == CommunicationType::Serial) {
        if (m_serialPortEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写串口名称。"));
            return;
        }
    } else if (t == CommunicationType::Tcp) {
        if (m_tcpHostEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写 TCP 主机地址。"));
            return;
        }
        if (m_tcpPortSpin->value() <= 0) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("TCP 端口必须在 1-65535。"));
            return;
        }
    } else if (t == CommunicationType::Udp) {
        if (m_udpHostEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写 UDP 主机地址。"));
            return;
        }
        if (m_udpPortSpin->value() <= 0) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("UDP 目标端口必须在 1-65535。"));
            return;
        }
    } else if (t == CommunicationType::Mqtt) {
        if (m_mqttBrokerHost->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写 MQTT Broker 地址。"));
            return;
        }
    }
    accept();
}
