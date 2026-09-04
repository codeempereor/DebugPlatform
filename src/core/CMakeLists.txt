# ==============================================================================
# 通信抽象层模块 (Core Module)
# ==============================================================================

set(CORE_SOURCES
    icommunication.cpp
    serialportcommunication.cpp
    tcpcommunication.cpp
    udpcommunication.cpp
    mqttcommunication.cpp
    communicationfactory.cpp
)

set(CORE_HEADERS
    ${INC_DIR}/core/icommunication.h
    ${INC_DIR}/core/serialportcommunication.h
    ${INC_DIR}/core/tcpcommunication.h
    ${INC_DIR}/core/udpcommunication.h
    ${INC_DIR}/core/mqttcommunication.h
    ${INC_DIR}/core/communicationfactory.h
    ${INC_DIR}/core/config.h
    ${INC_DIR}/core/communication_config.h
)

add_library(core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_link_libraries(core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Network
)