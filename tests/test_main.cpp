#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QSqlDatabase>
#include "include/test_crc16.h"
#include "include/test_binaryframe.h"
#include "include/test_modbus.h"
#include "include/test_datacache.h"
#include "include/test_alertmanager.h"
#include "include/test_ringbuffer.h"
#include "include/test_deviceconfig.h"
#include "include/test_datapersistent.h"
#include "include/test_logmanager.h"
#include "include/test_playbackengine.h"
#include "include/test_reconnectcontroller.h"
#include "include/test_communicationfactory.h"
#include "include/test_icommunication.h"
#include "include/test_tcpcommunication.h"
#include "include/test_udpcommunication.h"
#include "include/test_mqttcommunication.h"
#include "include/test_mqttprotocol.h"
#include "include/test_packetparserhighlighter.h"
#include "include/test_serialportcommunication.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    int testResult = 0;
    
    // TestCrc16
    char arg0[] = "DebugPlatformTests";
    char arg_out1[] = "-o";
    char arg_file1[] = "result_crc16.xml,xunitxml";
    char *args1[] = { arg0, arg_out1, arg_file1 };
    testResult |= QTest::qExec(new TestCrc16(), 3, args1);
    
    // TestBinaryFrame
    char arg_out2[] = "-o";
    char arg_file2[] = "result_binaryframe.xml,xunitxml";
    char *args2[] = { arg0, arg_out2, arg_file2 };
    testResult |= QTest::qExec(new TestBinaryFrame(), 3, args2);
    
    // TestModbus
    char arg_out3[] = "-o";
    char arg_file3[] = "result_modbus.xml,xunitxml";
    char *args3[] = { arg0, arg_out3, arg_file3 };
    testResult |= QTest::qExec(new TestModbus(), 3, args3);
    
    // TestDataCache
    char arg_out4[] = "-o";
    char arg_file4[] = "result_datacache.xml,xunitxml";
    char *args4[] = { arg0, arg_out4, arg_file4 };
    testResult |= QTest::qExec(new TestDataCache(), 3, args4);
    
    // TestAlertManager
    char arg_out5[] = "-o";
    char arg_file5[] = "result_alertmanager.xml,xunitxml";
    char *args5[] = { arg0, arg_out5, arg_file5 };
    testResult |= QTest::qExec(new TestAlertManager(), 3, args5);
    
    // TestRingBuffer
    char arg_out6[] = "-o";
    char arg_file6[] = "result_ringbuffer.xml,xunitxml";
    char *args6[] = { arg0, arg_out6, arg_file6 };
    testResult |= QTest::qExec(new TestRingBuffer(), 3, args6);
    
    // TestDeviceConfig
    char arg_out7[] = "-o";
    char arg_file7[] = "result_deviceconfig.xml,xunitxml";
    char *args7[] = { arg0, arg_out7, arg_file7 };
    testResult |= QTest::qExec(new TestDeviceConfig(), 3, args7);
    
    // TestDataPersistent
    char arg_out8[] = "-o";
    char arg_file8[] = "result_datapersistent.xml,xunitxml";
    char *args8[] = { arg0, arg_out8, arg_file8 };
    testResult |= QTest::qExec(new TestDataPersistent(), 3, args8);
    
    // TestLogManager
    char arg_out9[] = "-o";
    char arg_file9[] = "result_logmanager.xml,xunitxml";
    char *args9[] = { arg0, arg_out9, arg_file9 };
    testResult |= QTest::qExec(new TestLogManager(), 3, args9);
    
    // TestPlaybackEngine
    char arg_out10[] = "-o";
    char arg_file10[] = "result_playbackengine.xml,xunitxml";
    char *args10[] = { arg0, arg_out10, arg_file10 };
    testResult |= QTest::qExec(new TestPlaybackEngine(), 3, args10);
    
    // TestReconnectController
    char arg_out11[] = "-o";
    char arg_file11[] = "result_reconnectcontroller.xml,xunitxml";
    char *args11[] = { arg0, arg_out11, arg_file11 };
    testResult |= QTest::qExec(new TestReconnectController(), 3, args11);
    
    // TestCommunicationFactory
    char arg_out12[] = "-o";
    char arg_file12[] = "result_communicationfactory.xml,xunitxml";
    char *args12[] = { arg0, arg_out12, arg_file12 };
    testResult |= QTest::qExec(new TestCommunicationFactory(), 3, args12);
    
    // TestICommunication
    char arg_out13[] = "-o";
    char arg_file13[] = "result_icommunication.xml,xunitxml";
    char *args13[] = { arg0, arg_out13, arg_file13 };
    testResult |= QTest::qExec(new TestICommunication(), 3, args13);
    // TestTcpCommunication
    char arg_out14[] = "-o";
    char arg_file14[] = "result_tcpcommunication.xml,xunitxml";
    char *args14[] = { arg0, arg_out14, arg_file14 };
    testResult |= QTest::qExec(new TestTcpCommunication(), 3, args14);
    
    // TestUdpCommunication
    char arg_out15[] = "-o";
    char arg_file15[] = "result_udpcommunication.xml,xunitxml";
    char *args15[] = { arg0, arg_out15, arg_file15 };
    testResult |= QTest::qExec(new TestUdpCommunication(), 3, args15);
    
    // TestMqttCommunication
    char arg_out16[] = "-o";
    char arg_file16[] = "result_mqttcommunication.xml,xunitxml";
    char *args16[] = { arg0, arg_out16, arg_file16 };
    testResult |= QTest::qExec(new TestMqttCommunication(), 3, args16);
    
    // TestMqttProtocol
    char arg_out17[] = "-o";
    char arg_file17[] = "result_mqttprotocol.xml,xunitxml";
    char *args17[] = { arg0, arg_out17, arg_file17 };
    testResult |= QTest::qExec(new TestMqttProtocol(), 3, args17);
    
    // TestPacketParserHighlighter
    char arg_out18[] = "-o";
    char arg_file18[] = "result_packetparserhighlighter.xml,xunitxml";
    char *args18[] = { arg0, arg_out18, arg_file18 };
    testResult |= QTest::qExec(new TestPacketParserHighlighter(), 3, args18);

    // TestSerialPortCommunication
    char arg_out19[] = "-o";
    char arg_file19[] = "result_serialportcommunication.xml,xunitxml";
    char *args19[] = { arg0, arg_out19, arg_file19 };
    testResult |= QTest::qExec(new TestSerialPortCommunication(), 3, args19);

    return testResult;
}