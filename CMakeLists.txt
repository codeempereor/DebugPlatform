# ==============================================================================
# 多协议嵌入式设备调试平台 (Multi-Protocol Embedded Debug Platform)
# ==============================================================================
# CMake 最低版本要求
cmake_minimum_required(VERSION 3.16)

# 项目信息
project(DebugPlatform
    VERSION 1.2.0
    DESCRIPTION "Multi-Protocol Embedded Device Debug Platform"
    LANGUAGES CXX
)

# C++ 标准（与 qmake 工程保持一致）
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)       # 自动处理 MOC（Meta-Object Compiler）
set(CMAKE_AUTORCC ON)       # 自动处理资源文件
set(CMAKE_AUTOUIC ON)       # 自动处理 UI 文件

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 可选：Clang-Tidy 静态分析（启用方式：cmake -DENABLE_CLANG_TIDY=ON ..）
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_EXE}"
            "-checks=bugprone-*,performance-*,modernize-*,readability-*,-modernize-use-trailing-return-type"
            "-warnings-as-errors=bugprone-*,performance-*"
        )
        message(STATUS "Clang-Tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
endif()

# ==============================================================================
# Qt 模块查找 (Qt 5.14+ / Qt 6.x 双兼容)
# ==============================================================================
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    SerialPort
    Network
    Sql
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    SerialPort
    Network
    Sql
)

# ==============================================================================
# 目录定义
# ==============================================================================
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(RES_DIR ${CMAKE_SOURCE_DIR}/resources)

# ==============================================================================
# 公共头文件目录
# ==============================================================================
include_directories(${INC_DIR})
include_directories(${INC_DIR}/core)
include_directories(${INC_DIR}/protocol)
include_directories(${INC_DIR}/data)
include_directories(${INC_DIR}/device)
include_directories(${INC_DIR}/ui)
include_directories(${INC_DIR}/app)

# ==============================================================================
# 子模块
# ==============================================================================
add_subdirectory(src/core)        # 通信抽象层
add_subdirectory(src/protocol)    # 协议解析层
add_subdirectory(src/data)        # 数据管理层
add_subdirectory(src/device)      # 多设备管理
add_subdirectory(src/ui)          # UI 层
add_subdirectory(src/app)         # 主程序入口

# ==============================================================================
# 资源文件（AUTORCC 自动处理 .qrc，无需手动 qt_add_resources）
# ==============================================================================
file(GLOB RES_FILES ${RES_DIR}/*.qrc)

# ==============================================================================
# 安装规则
# ==============================================================================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# ==============================================================================
# 打印配置信息
# ==============================================================================
message(STATUS "")
message(STATUS "============================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "============================================")
message(STATUS "  C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt Version:      ${Qt${QT_VERSION_MAJOR}_VERSION}")
message(STATUS "  Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:        ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "============================================")
message(STATUS "")
