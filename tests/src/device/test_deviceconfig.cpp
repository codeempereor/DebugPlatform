#include "test_deviceconfig.h"

void TestDeviceConfig::initTestCase()
{
    qDebug() << "=== DeviceConfig 测试套件开始 ===";
}

void TestDeviceConfig::cleanupTestCase()
{
    qDebug() << "=== DeviceConfig 测试套件结束 ===";
}

void TestDeviceConfig::testDefaultConfig()
{
    DeviceConfig config;
    
    QVERIFY(config.id.isEmpty());
    QVERIFY(config.name.isEmpty());
    QCOMPARE(config.commType, CommunicationType::Serial);
    QCOMPARE(config.protocolType, ProtocolType::BinaryFrame);
    QCOMPARE(config.pollInterval, 100);
    QCOMPARE(config.timeout, 1000);
    QVERIFY(!config.autoReconnect);
    QVERIFY(config.enabled);
    
    qDebug() << "默认配置测试通过";
}

void TestDeviceConfig::testSetProperties()
{
    DeviceConfig config;
    config.id = "test001";
    config.name = "测试设备";
    config.pollInterval = 500;
    config.timeout = 2000;
    config.autoReconnect = true;
    config.enabled = false;
    
    QCOMPARE(config.id, QString("test001"));
    QCOMPARE(config.name, QString("测试设备"));
    QCOMPARE(config.pollInterval, 500);
    QCOMPARE(config.timeout, 2000);
    QVERIFY(config.autoReconnect);
    QVERIFY(!config.enabled);
    
    qDebug() << "属性设置测试通过";
}

void TestDeviceConfig::testToMapAndFromMap()
{
    DeviceConfig config;
    config.id = "test002";
    config.name = "序列化测试";
    config.commType = CommunicationType::Tcp;
    config.protocolType = ProtocolType::ModbusTcp;
    config.pollInterval = 200;
    config.timeout = 1500;
    config.autoReconnect = true;
    
    // 设置 TCP 配置
    config.commConfig.tcp.hostAddress = "127.0.0.1";
    config.commConfig.tcp.port = 502;
    
    // 序列化
    QVariantMap map = config.toMap();
    
    // 反序列化到新配置
    DeviceConfig restored;
    restored.fromMap(map);
    
    // 验证
    QCOMPARE(restored.id, config.id);
    QCOMPARE(restored.name, config.name);
    QCOMPARE(restored.commType, config.commType);
    QCOMPARE(restored.protocolType, config.protocolType);
    QCOMPARE(restored.pollInterval, config.pollInterval);
    QCOMPARE(restored.timeout, config.timeout);
    QVERIFY(restored.autoReconnect);
    QCOMPARE(restored.commConfig.tcp.hostAddress, QString("127.0.0.1"));
    QCOMPARE(restored.commConfig.tcp.port, static_cast<quint16>(502));
    
    qDebug() << "序列化/反序列化测试通过";
}

void TestDeviceConfig::testSerialConfig()
{
    DeviceConfig config;
    config.commType = CommunicationType::Serial;
    config.commConfig.serial.portName = "COM9";
    config.commConfig.serial.baudRate = 115200;
    config.commConfig.serial.dataBits = QSerialPort::Data8;
    config.commConfig.serial.stopBits = QSerialPort::OneStop;
    config.commConfig.serial.parity = QSerialPort::NoParity;
    
    QCOMPARE(config.commConfig.serial.portName, QString("COM9"));
    QCOMPARE(config.commConfig.serial.baudRate, 115200);
    QCOMPARE(config.commConfig.serial.dataBits, QSerialPort::Data8);
    
    qDebug() << "串口配置测试通过";
}

void TestDeviceConfig::testTcpConfig()
{
    DeviceConfig config;
    config.commType = CommunicationType::Tcp;
    config.commConfig.tcp.hostAddress = "192.168.1.100";
    config.commConfig.tcp.port = 5020;
    
    QCOMPARE(config.commConfig.tcp.hostAddress, QString("192.168.1.100"));
    QCOMPARE(config.commConfig.tcp.port, static_cast<quint16>(5020));
    
    qDebug() << "TCP 配置测试通过";
}

void TestDeviceConfig::testUdpConfig()
{
    DeviceConfig config;
    config.commType = CommunicationType::Udp;
    config.commConfig.udp.hostAddress = "127.0.0.1";
    config.commConfig.udp.port = 503;
    config.commConfig.udp.localPort = 0;
    
    QCOMPARE(config.commConfig.udp.hostAddress, QString("127.0.0.1"));
    QCOMPARE(config.commConfig.udp.port, static_cast<quint16>(503));
    QCOMPARE(config.commConfig.udp.localPort, static_cast<quint16>(0));
    
    qDebug() << "UDP 配置测试通过";
}

void TestDeviceConfig::testToString()
{
    DeviceConfig config;
    config.id = "test003";
    config.name = "字符串测试";
    config.commType = CommunicationType::Tcp;
    config.commConfig.type = CommunicationType::Tcp;
    config.commConfig.tcp.hostAddress = "127.0.0.1";
    config.commConfig.tcp.port = 502;
    
    QString str = config.toString();
    
    QVERIFY(str.contains("test003"));
    QVERIFY(str.contains("字符串测试"));
    QVERIFY(str.contains("TCP"));
    
    qDebug() << "toString 测试通过:" << str;
}

