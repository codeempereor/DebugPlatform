# ==============================================================================
# 通信抽象层 (Core Module)
# ==============================================================================

TEMPLATE = lib
CONFIG += staticlib c++11
QT += core serialport network

TARGET = core

# 头文件
INCLUDEPATH += \
    $$PWD/../../include \
    $$PWD/../../include/core

HEADERS += \
    $$PWD/../../include/core/config.h \
    $$PWD/../../include/core/communication_config.h \
    $$PWD/../../include/core/icommunication.h \
    $$PWD/../../include/core/serialportcommunication.h \
    $$PWD/../../include/core/tcpcommunication.h \
    $$PWD/../../include/core/udpcommunication.h \
    $$PWD/../../include/core/communicationfactory.h

# 源文件
SOURCES += \
    icommunication.cpp \
    serialportcommunication.cpp \
    tcpcommunication.cpp \
    udpcommunication.cpp \
    communicationfactory.cpp
