# ==============================================================================
# 数据管理层 (Data Module)
# ==============================================================================

TEMPLATE = lib
CONFIG += staticlib c++11
QT += core sql network serialport

TARGET = data

# 头文件
INCLUDEPATH += \
    $$PWD/../../include \
    $$PWD/../../include/data \
    $$PWD/../../include/core

HEADERS += \
    $$PWD/../../include/data/ringbuffer.h \
    $$PWD/../../include/data/datacache.h \
    $$PWD/../../include/data/datapersistent.h \
    $$PWD/../../include/data/logmanager.h \
    $$PWD/../../include/data/alertmanager.h \
    $$PWD/../../include/data/playbackengine.h

# 源文件
SOURCES += \
    datacache.cpp \
    datapersistent.cpp \
    logmanager.cpp \
    alertmanager.cpp \
    playbackengine.cpp
