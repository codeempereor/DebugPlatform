# ==============================================================================
# 协议解析层 (Protocol Module)
# ==============================================================================

TEMPLATE = lib
CONFIG += staticlib c++11
QT += core

TARGET = protocol

# 头文件
INCLUDEPATH += \
    $$PWD/../../include \
    $$PWD/../../include/protocol \
    $$PWD/../../include/core

HEADERS += \
    $$PWD/../../include/protocol/crc16.h \
    $$PWD/../../include/protocol/binaryframeparser.h \
    $$PWD/../../include/protocol/binaryframebuilder.h \
    $$PWD/../../include/protocol/modbusrtuprotocol.h \
    $$PWD/../../include/protocol/modbustcpprotocol.h

# 源文件
SOURCES += \
    crc16.cpp \
    binaryframeparser.cpp \
    binaryframebuilder.cpp \
    modbusrtuprotocol.cpp \
    modbustcpprotocol.cpp
