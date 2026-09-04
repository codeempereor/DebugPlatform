#ifndef DEBUGPLATFORM_COMMANDPANELWIDGET_H
#define DEBUGPLATFORM_COMMANDPANELWIDGET_H

/**
 * @file commandpanelwidget.h
 * @brief 命令发送面板
 *
 * 提供三种交互方式：
 *   1) 自定义二进制帧：输入十六进制字节，预览 CRC，并支持重复发送
 *   2) Modbus 快捷操作：功能码/寄存器/值配置
 *   3) 发送历史：展示最近的命令与响应
 */

#include <QWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QByteArray>
#include <QStringList>
#include "core/config.h"

/**
 * @class CommandPanelWidget
 * @brief 命令发送面板
 */
class CommandPanelWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CommandPanelWidget(QWidget *parent = nullptr);

    /**
     * @brief 设置当前目标设备
     */
    void setDeviceId(const QString &deviceId);

    /**
     * @brief 向历史列表追加一条记录
     * @param command 命令十六进制字符串
     * @param response 响应十六进制字符串
     * @param responseTime 响应耗时（毫秒）
     */
    void appendHistory(const QString &command, const QString &response,
                      int responseTime);

signals:
    /**
     * @brief 请求发送原始二进制帧
     */
    void sendBinaryFrameRequested(const QByteArray &data);

    /**
     * @brief 请求发送 Modbus 命令
     * @param functionCode 功能码
     * @param address 寄存器地址
     * @param quantity 数量（读取或多写时使用）
     * @param value 值（单写时使用，或多写时的起始值）
     * @param slaveAddress 从站地址
     */
    void sendModbusCommandRequested(quint8 functionCode, quint16 address,
                                    quint16 quantity, quint16 value,
                                    quint8 slaveAddress);

    /**
     * @brief 请求清空历史
     */
    void sendHistoryCleared();

private slots:
    // Tab1 自定义二进制
    void onBinaryTextChanged();
    void onSendBinaryClicked();
    void onRepeatValueChanged(int value);

    // Tab2 Modbus
    void onModbusSendClicked();
    void onModbusFunctionChanged(int index);

    // Tab3 历史
    void onClearHistory();

private:
    /**
     * @brief 校验 hex 字符串并转为 QByteArray（空格允许）
     */
    static QByteArray fromHexString(const QString &hex);

    /**
     * @brief 计算 CRC16（Modbus 多项式），返回 2 字节小端序
     */
    static QByteArray crc16Modbus(const QByteArray &data);

    /**
     * @brief 更新 Tab1 预览标签
     */
    void updatePreview();

private:
    QTabWidget     *m_tabs = nullptr;

    // ===== Tab1: 自定义二进制 =====
    QLineEdit      *m_hexEdit      = nullptr;
    QLabel         *m_previewLabel = nullptr;
    QPushButton    *m_sendBtn      = nullptr;
    QSpinBox       *m_repeatSpin   = nullptr;
    QLabel         *m_targetLabel  = nullptr;

    // ===== Tab2: Modbus =====
    QComboBox      *m_functionCombo = nullptr;
    QSpinBox       *m_slaveSpin     = nullptr;
    QSpinBox       *m_regAddrSpin   = nullptr;
    QSpinBox       *m_quantitySpin = nullptr;
    QSpinBox       *m_valueSpin    = nullptr;
    QPushButton    *m_modbusSendBtn = nullptr;
    QLabel         *m_modbusHint   = nullptr;

    // ===== Tab3: 历史 =====
    QListWidget    *m_historyList  = nullptr;
    QPushButton    *m_clearHistBtn = nullptr;

    // 当前设备
    QString         m_deviceId;

    // Modbus 功能码枚举
    enum ModbusFunction {
        ReadCoils          = 0,  // 0x01
        ReadDiscreteInputs  = 1,  // 0x02
        ReadHoldingRegisters = 2, // 0x03
        ReadInputRegisters  = 3, // 0x04
        WriteSingleCoil     = 4, // 0x05
        WriteSingleRegister = 5, // 0x06
        WriteMultipleCoils  = 6, // 0x0F
        WriteMultipleRegisters = 7 // 0x10
    };
};

#endif // DEBUGPLATFORM_COMMANDPANELWIDGET_H
