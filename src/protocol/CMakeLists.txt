# 协议解析层模块
set(PROTOCOL_SOURCES
    binaryframeparser.cpp
    binaryframebuilder.cpp
    crc16.cpp
    modbusrtuprotocol.cpp
    modbustcpprotocol.cpp
    mqttprotocol.cpp
)

set(PROTOCOL_HEADERS
    ${INC_DIR}/protocol/binaryframeparser.h
    ${INC_DIR}/protocol/binaryframebuilder.h
    ${INC_DIR}/protocol/crc16.h
    ${INC_DIR}/protocol/modbusrtuprotocol.h
    ${INC_DIR}/protocol/modbustcpprotocol.h
    ${INC_DIR}/protocol/mqttprotocol.h
)

add_library(protocol STATIC
    ${PROTOCOL_SOURCES}
    ${PROTOCOL_HEADERS}
)

target_link_libraries(protocol PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)
