# DebugPlatform Unit Tests
QT += core testlib sql network serialport
CONFIG += c++11 testcase

# Coverage flags
QMAKE_CXXFLAGS += --coverage -fprofile-arcs -ftest-coverage
QMAKE_LFLAGS += --coverage
LIBS += -lgcov

TARGET = DebugPlatformTests
TEMPLATE = app

# Source files
SOURCES += \
    test_main.cpp \
    src/protocol/test_crc16.cpp \
    src/protocol/test_binaryframe.cpp \
    src/protocol/test_modbus.cpp \
    src/data/test_datacache.cpp \
    src/data/test_alertmanager.cpp \
    src/data/test_ringbuffer.cpp \
    src/device/test_deviceconfig.cpp \
    src/data/test_datapersistent.cpp \
    src/data/test_logmanager.cpp \
    src/data/test_playbackengine.cpp \
    src/device/test_reconnectcontroller.cpp \
    src/core/test_communicationfactory.cpp \
    src/core/test_icommunication.cpp \
    src/core/test_tcpcommunication.cpp \
    src/core/test_mqttcommunication.cpp \
    src/core/test_udpcommunication.cpp \
    src/protocol/test_mqttprotocol.cpp \
    src/ui/test_packetparserhighlighter.cpp \
    src/core/test_serialportcommunication.cpp

# Source under test
SOURCES += \
    ../src/protocol/crc16.cpp \
    ../src/protocol/binaryframebuilder.cpp \
    ../src/protocol/binaryframeparser.cpp \
    ../src/protocol/modbusrtuprotocol.cpp \
    ../src/protocol/modbustcpprotocol.cpp \
    ../src/protocol/mqttprotocol.cpp \
    ../src/data/datacache.cpp \
    ../src/data/alertmanager.cpp \
    ../src/data/logmanager.cpp \
    ../src/data/datapersistent.cpp \
    ../src/data/playbackengine.cpp \
    ../src/device/deviceconfig.cpp \
    ../src/device/reconnectcontroller.cpp \
    ../src/core/communicationfactory.cpp \
    ../src/core/icommunication.cpp \
    ../src/core/serialportcommunication.cpp \
    ../src/core/tcpcommunication.cpp \
    ../src/core/udpcommunication.cpp \
    ../src/core/mqttcommunication.cpp \
    ../src/ui/packetparserhighlighter.cpp

# Include paths
INCLUDEPATH += \
    ../include \
    ../include/protocol \
    ../include/data \
    ../include/device \
    ../include/core \
    ../include/app \
    ../src/protocol \
    ../src/data \
    ../src/device \
    ../src/core \
    include

# Q_OBJECT headers for MOC processing
HEADERS += \
    ../include/protocol/binaryframeparser.h \
    ../include/data/datacache.h \
    ../include/data/alertmanager.h \
    ../include/data/datapersistent.h \
    ../include/data/logmanager.h \
    ../include/data/playbackengine.h \
    ../include/device/reconnectcontroller.h \
    ../include/core/icommunication.h \
    ../include/core/serialportcommunication.h \
    ../include/core/tcpcommunication.h \
    ../include/core/udpcommunication.h \
    ../include/core/mqttcommunication.h \
    ../include/ui/packetparserhighlighter.h \
    include/test_crc16.h \
    include/test_binaryframe.h \
    include/test_modbus.h \
    include/test_datacache.h \
    include/test_alertmanager.h \
    include/test_ringbuffer.h \
    include/test_deviceconfig.h \
    include/test_datapersistent.h \
    include/test_logmanager.h \
    include/test_playbackengine.h \
    include/test_reconnectcontroller.h \
    include/test_communicationfactory.h \
    include/test_icommunication.h \
    include/test_tcpcommunication.h \
    include/test_mqttcommunication.h \
    include/test_udpcommunication.h \
    include/test_mqttprotocol.h \
    include/test_packetparserhighlighter.h \
    include/test_serialportcommunication.h
