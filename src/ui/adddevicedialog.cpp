#include "ui/adddevicedialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QSerialPort>
#include <QUuid>

AddDeviceDialog::AddDeviceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("添加设备"));
    resize(440, 480);

    auto *mainLayout = new QVBoxLayout(this);

    // ============ 基础信息 ============
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
    m_protocolCombo->addItem(tr("BinaryFrame"),     static_cast<int>(ProtocolType::BinaryFrame));
    m_protocolCombo->addItem(tr("Modbus RTU"),      static_cast<int>(ProtocolType::ModbusRtu));
    m_protocolCombo->addItem(tr("Modbus TCP"),      static_cast<int>(ProtocolType::ModbusTcp));
    m_protocolCombo->addItem(tr("MQTT JSON"),       static_cast<int>(ProtocolType::MqttJson));
    baseForm->addRow(tr("协议类型："), m_protocolCombo);

    mainLayout->addWidget(baseGroup);

    // ============ 通信配置容器 ============
    m_serialFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_serialFrame);
        form->setContentsMargins(4, 4, 4, 4);

        m_serialPortEdit = new QLineEdit(m_serialFrame);
        m_serialPortEdit->setPlaceholderText(tr("COM1 / /dev/ttyUSB0"));
        form->addRow(tr("端口："), m_serialPortEdit);

        m_serialBaudCombo = new QComboBox(m_serialFrame);
        const QList<qint32> bauds = {9600, 19200, 38400, 57600, 115200, 230400, 460800};
        for (qint32 b : bauds) m_serialBaudCombo->addItem(QString::number(b), b);
        m_serialBaudCombo->setCurrentIndex(4); // 115200
        form->addRow(tr("波特率："), m_serialBaudCombo);

        m_serialDataBits = new QComboBox(m_serialFrame);
        m_serialDataBits->addItem(QStringLiteral("5"),
            static_cast<int>(QSerialPort::Data5));
        m_serialDataBits->addItem(QStringLiteral("6"),
            static_cast<int>(QSerialPort::Data6));
        m_serialDataBits->addItem(QStringLiteral("7"),
            static_cast<int>(QSerialPort::Data7));
        m_serialDataBits->addItem(QStringLiteral("8"),
            static_cast<int>(QSerialPort::Data8));
        m_serialDataBits->setCurrentIndex(3);
        form->addRow(tr("数据位："), m_serialDataBits);

        m_serialStopBits = new QComboBox(m_serialFrame);
        m_serialStopBits->addItem(QStringLiteral("1"),
            static_cast<int>(QSerialPort::OneStop));
        m_serialStopBits->addItem(QStringLiteral("1.5"),
            static_cast<int>(QSerialPort::OneAndHalfStop));
        m_serialStopBits->addItem(QStringLiteral("2"),
            static_cast<int>(QSerialPort::TwoStop));
        form->addRow(tr("停止位："), m_serialStopBits);

        m_serialParity = new QComboBox(m_serialFrame);
        m_serialParity->addItem(tr("None"), static_cast<int>(QSerialPort::NoParity));
        m_serialParity->addItem(tr("Even"), static_cast<int>(QSerialPort::EvenParity));
        m_serialParity->addItem(tr("Odd"),  static_cast<int>(QSerialPort::OddParity));
        m_serialParity->addItem(tr("Space"), static_cast<int>(QSerialPort::SpaceParity));
        m_serialParity->addItem(tr("Mark"), static_cast<int>(QSerialPort::MarkParity));
        form->addRow(tr("校验位："), m_serialParity);
    }

    m_tcpFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_tcpFrame);
        form->setContentsMargins(4, 4, 4, 4);

        m_tcpHostEdit = new QLineEdit(m_tcpFrame);
        m_tcpHostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
        form->addRow(tr("主机地址："), m_tcpHostEdit);

        m_tcpPortSpin = new QSpinBox(m_tcpFrame);
        m_tcpPortSpin->setRange(1, 65535);
        m_tcpPortSpin->setValue(502);
        form->addRow(tr("端口："), m_tcpPortSpin);
    }

    m_udpFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_udpFrame);
        form->setContentsMargins(4, 4, 4, 4);

        m_udpHostEdit = new QLineEdit(m_udpFrame);
        m_udpHostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
        form->addRow(tr("主机地址："), m_udpHostEdit);

        m_udpPortSpin = new QSpinBox(m_udpFrame);
        m_udpPortSpin->setRange(1, 65535);
        m_udpPortSpin->setValue(502);
        form->addRow(tr("端口："), m_udpPortSpin);

        m_udpLocalPort = new QSpinBox(m_udpFrame);
        m_udpLocalPort->setRange(0, 65535);
        m_udpLocalPort->setValue(0);
        m_udpLocalPort->setSpecialValueText(tr("自动分配"));
        form->addRow(tr("本地端口："), m_udpLocalPort);
    }

    m_mqttFrame = new QWidget(this);
    {
        auto *form = new QFormLayout(m_mqttFrame);
        form->setContentsMargins(4, 4, 4, 4);

        m_mqttBrokerHost = new QLineEdit(m_mqttFrame);
        m_mqttBrokerHost->setPlaceholderText(QStringLiteral("broker.emqx.io"));
        m_mqttBrokerHost->setText(QStringLiteral("broker.emqx.io"));
        form->addRow(tr("Broker 地址："), m_mqttBrokerHost);

        m_mqttBrokerPort = new QSpinBox(m_mqttFrame);
        m_mqttBrokerPort->setRange(1, 65535);
        m_mqttBrokerPort->setValue(1883);
        form->addRow(tr("Broker 端口："), m_mqttBrokerPort);

        m_mqttClientId = new QLineEdit(m_mqttFrame);
        m_mqttClientId->setPlaceholderText(tr("留空则自动生成 DebugPlatform_xxx"));
        form->addRow(tr("Client ID："), m_mqttClientId);

        m_mqttUsername = new QLineEdit(m_mqttFrame);
        m_mqttUsername->setPlaceholderText(tr("可选（匿名 Broker 留空）"));
        form->addRow(tr("用户名："), m_mqttUsername);

        m_mqttPassword = new QLineEdit(m_mqttFrame);
        m_mqttPassword->setEchoMode(QLineEdit::Password);
        m_mqttPassword->setPlaceholderText(tr("可选"));
        form->addRow(tr("密码："), m_mqttPassword);

        m_mqttSubTopic = new QLineEdit(m_mqttFrame);
        m_mqttSubTopic->setPlaceholderText(QStringLiteral("debug/platform/devA/data"));
        m_mqttSubTopic->setText(QStringLiteral("debug/platform/+/data/#")); // 预填：覆盖 /data + /data/5 等
        form->addRow(tr("订阅主题："), m_mqttSubTopic);

        m_mqttPubTopic = new QLineEdit(m_mqttFrame);
        m_mqttPubTopic->setPlaceholderText(QStringLiteral("debug/platform/DebugPlatform/cmd"));
        m_mqttPubTopic->setText(QStringLiteral("debug/platform/DebugPlatform/cmd")); // 预填
        form->addRow(tr("发布主题："), m_mqttPubTopic);

        m_mqttQosCombo = new QComboBox(m_mqttFrame);
        m_mqttQosCombo->addItem(QStringLiteral("QoS 0 - 最多一次"), 0);
        m_mqttQosCombo->addItem(QStringLiteral("QoS 1 - 至少一次"), 1);
        m_mqttQosCombo->addItem(QStringLiteral("QoS 2 - 恰好一次"), 2);
        m_mqttQosCombo->setCurrentIndex(0);
        form->addRow(tr("QoS 级别："), m_mqttQosCombo);

        m_mqttKeepAlive = new QSpinBox(m_mqttFrame);
        m_mqttKeepAlive->setRange(5, 3600);
        m_mqttKeepAlive->setValue(60);
        m_mqttKeepAlive->setSuffix(QStringLiteral(" 秒"));
        form->addRow(tr("KeepAlive："), m_mqttKeepAlive);

        m_mqttCleanSession = new QCheckBox(tr("Clean Session（启动时清除之前会话）"), m_mqttFrame);
        m_mqttCleanSession->setChecked(true);
        form->addRow(QString(), m_mqttCleanSession);
    }

    auto *commGroup = new QGroupBox(tr("通信配置"), this);
    auto *commLayout = new QVBoxLayout(commGroup);
    commLayout->addWidget(m_serialFrame);
    commLayout->addWidget(m_tcpFrame);
    commLayout->addWidget(m_udpFrame);
    commLayout->addWidget(m_mqttFrame);
    mainLayout->addWidget(commGroup);

    // ============ 轮询参数 ============
    auto *pollGroup = new QGroupBox(tr("轮询与超时"), this);
    auto *pollForm = new QFormLayout(pollGroup);

    m_pollIntervalSpin = new QSpinBox(pollGroup);
    m_pollIntervalSpin->setRange(1, 60000);
    m_pollIntervalSpin->setValue(100);
    m_pollIntervalSpin->setSuffix(QStringLiteral(" ms"));
    pollForm->addRow(tr("轮询间隔："), m_pollIntervalSpin);

    m_timeoutSpin = new QSpinBox(pollGroup);
    m_timeoutSpin->setRange(1, 60000);
    m_timeoutSpin->setValue(1000);
    m_timeoutSpin->setSuffix(QStringLiteral(" ms"));
    pollForm->addRow(tr("超时："), m_timeoutSpin);

    m_autoReconnectCheck = new QCheckBox(tr("自动重连"), pollGroup);
    m_autoReconnectCheck->setChecked(false);
    pollForm->addRow(QString(), m_autoReconnectCheck);

    mainLayout->addWidget(pollGroup);

    // ============ 按钮 ============
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_commTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddDeviceDialog::onCommTypeChanged);
    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddDeviceDialog::onProtocolTypeChanged);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &AddDeviceDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    updateCommConfigVisibility();
}

// ============================================================================
// 公共 API
// ============================================================================

DeviceConfig AddDeviceDialog::getConfig() const
{
    DeviceConfig cfg;
    cfg.id   = QUuid::createUuid().toString(QUuid::WithoutBraces);
    cfg.name = m_nameEdit->text().trimmed();

    const auto commType = static_cast<CommunicationType>(
        m_commTypeCombo->currentData().toInt());
    cfg.commType = commType;
    cfg.protocolType = static_cast<ProtocolType>(
        m_protocolCombo->currentData().toInt());
    cfg.pollInterval = m_pollIntervalSpin->value();
    cfg.timeout      = m_timeoutSpin->value();
    cfg.autoReconnect = m_autoReconnectCheck->isChecked();

    switch (commType) {
    case CommunicationType::Serial:
        cfg.commConfig.type = CommunicationType::Serial;
        cfg.commConfig.serial.portName    = m_serialPortEdit->text().trimmed();
        cfg.commConfig.serial.baudRate    = m_serialBaudCombo->currentData().toInt();
        cfg.commConfig.serial.dataBits    = static_cast<QSerialPort::DataBits>(
            m_serialDataBits->currentData().toInt());
        cfg.commConfig.serial.stopBits    = static_cast<QSerialPort::StopBits>(
            m_serialStopBits->currentData().toInt());
        cfg.commConfig.serial.parity      = static_cast<QSerialPort::Parity>(
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
// 槽
// ============================================================================

void AddDeviceDialog::onCommTypeChanged(int index)
{
    Q_UNUSED(index);
    updateCommConfigVisibility();
}

void AddDeviceDialog::onProtocolTypeChanged(int index)
{
    // Modbus RTU 自动选择 Serial；Modbus TCP 自动选择 TCP；保持可手动调整
    const auto proto = static_cast<ProtocolType>(
        m_protocolCombo->currentData().toInt());
    if (proto == ProtocolType::ModbusRtu) {
        m_commTypeCombo->setCurrentIndex(
            m_commTypeCombo->findData(static_cast<int>(CommunicationType::Serial)));
    } else if (proto == ProtocolType::ModbusTcp) {
        m_commTypeCombo->setCurrentIndex(
            m_commTypeCombo->findData(static_cast<int>(CommunicationType::Tcp)));
    } else if (proto == ProtocolType::MqttJson) {
        m_commTypeCombo->setCurrentIndex(
            m_commTypeCombo->findData(static_cast<int>(CommunicationType::Mqtt)));
    }
    Q_UNUSED(index);
}

void AddDeviceDialog::onAccepted()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("校验失败"), tr("请填写设备名称。"));
        return;
    }
    const auto t = static_cast<CommunicationType>(
        m_commTypeCombo->currentData().toInt());
    if (t == CommunicationType::Serial) {
        const QString port = m_serialPortEdit->text().trimmed();
        if (port.isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写串口名称，例如 COM1 或 /dev/ttyUSB0。"));
            return;
        }
    } else if (t == CommunicationType::Tcp) {
        const QString host = m_tcpHostEdit->text().trimmed();
        if (host.isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写 TCP 主机地址。"));
            return;
        }
        if (m_tcpPortSpin->value() <= 0) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("TCP 端口范围必须为 1-65535。"));
            return;
        }
    } else if (t == CommunicationType::Udp) {
        const QString host = m_udpHostEdit->text().trimmed();
        if (host.isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写 UDP 主机地址。"));
            return;
        }
        if (m_udpPortSpin->value() <= 0) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("UDP 目标端口范围必须为 1-65535。"));
            return;
        }
    } else if (t == CommunicationType::Mqtt) {
        const QString broker = m_mqttBrokerHost->text().trimmed();
        if (broker.isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写 MQTT Broker 地址。"));
            return;
        }
        if (m_mqttBrokerPort->value() <= 0) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("MQTT Broker 端口必须为 1-65535。"));
            return;
        }
        const QString sub = m_mqttSubTopic->text().trimmed();
        if (sub.isEmpty()) {
            QMessageBox::warning(this, tr("校验失败"),
                tr("请填写订阅主题（例如 debug/platform/+/data）。"));
            return;
        }
    }
    accept();
}

// ============================================================================
// 私有辅助
// ============================================================================

void AddDeviceDialog::updateCommConfigVisibility()
{
    const auto t = static_cast<CommunicationType>(
        m_commTypeCombo->currentData().toInt());
    m_serialFrame->setVisible(t == CommunicationType::Serial);
    m_tcpFrame->setVisible(t == CommunicationType::Tcp);
    m_udpFrame->setVisible(t == CommunicationType::Udp);
    m_mqttFrame->setVisible(t == CommunicationType::Mqtt);
}
