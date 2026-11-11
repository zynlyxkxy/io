project(CTL_MDL_IO VERSION 1.0.0.0)

# ============================================================
#  本地仿真模式
#
#  打开：ACS / PLC 走本地仿真实现，不需要真实运动控制卡和 PLC
#  关闭：本模块编译为空壳，所有接口返回默认值 —— 接真实硬件时用
#
#  注意作用域：这是 directory 作用域，只覆盖 MainProject/ 及其子目录，
#  所以只有 CTL_MDL_IO 带这个宏，GUI_MDL_IO 与 io 库都不带。
#  切换方式：解开/加上下面那行的注释即可（与 motion 的
#  MOTIONSTAGE_LOCAL_DEBUG、mctrl 的 VisionCF_LOCAL_DEBUG 同一套做法）。
# ============================================================

# add_compile_definitions(IO_LOCAL_DEBUG)

set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

set(MAIN_SOURCES
    main.cpp
    ControlModule/ConfigManager/ConfigManager.cpp
    ControlModule/Controller/IoController.cpp
    ControlModule/IoServer/IoServer.cpp
    AcsModule/AcsIoModule/AcsIoModule.cpp
    AcsModule/AcsClient/AcsClient.cpp
    PlcModule/PlcIoModule/PlcIoModule.cpp
    PlcModule/PlcClient/PlcClient.cpp
)

set(MAIN_HEADERS
    ControlModule/ConfigManager/ConfigManager.h
    ControlModule/ConfigManager/data/ConfigData.h
    ControlModule/Controller/IoController.h
    ControlModule/IoServer/IoServer.h
    AcsModule/AcsIoModule/AcsIoModule.h
    AcsModule/AcsClient/AcsClient.h
    PlcModule/PlcIoModule/PlcIoModule.h
    PlcModule/PlcClient/PlcClient.h
    ${CMAKE_SOURCE_DIR}/interface/include/ioc_data.h
)

source_group("Source Files" FILES ${MAIN_SOURCES})
source_group("Header Files" FILES ${MAIN_HEADERS})

istar_add_app(${PROJECT_NAME} SOURCES ${MAIN_SOURCES} ${MAIN_HEADERS})

target_compile_definitions(${PROJECT_NAME} PUBLIC ISTAR_BUILDING)

target_include_directories(${PROJECT_NAME} BEFORE PUBLIC
    ${CMAKE_SOURCE_DIR}/interface/include
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/AcsModule
    ${CMAKE_CURRENT_SOURCE_DIR}/PlcModule
    ${CMAKE_CURRENT_SOURCE_DIR}/AcsModule/AcsClient/acs/include
    ${BASIC_INC}/common/include
    ${BASIC_INC}/istarlog/include
    ${BASIC_INC}/istarmw/include
    ${BASIC_INC}/istaryaml/include
    ${BASIC_INC}/istargf/include
    ${BASIC_INC}/istarthreadpool/include
    ${BASIC_INC}/istarserializer/include
    ${BASIC_INC}/istarsm/include
)

# ACSCL_x64 / WibuCm64 是运动控制卡厂商库，随源码入库（不在平台基础库里）
set(ACS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/AcsModule/AcsClient/acs/lib")

target_link_directories(${PROJECT_NAME} PUBLIC
    ${BASIC_LIB}
    ${ACS_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    io                           # 同工程内的协议库，直接 link target（不再走 interface/lib 路径）
    debug ISTARMWd         optimized ISTARMW
    debug ISTARYamld       optimized ISTARYaml
    debug ISTARLogd        optimized ISTARLog
    debug ISTARGFd         optimized ISTARGF
    debug ISTARThreadPoold optimized ISTARThreadPool
    debug ISTARSerializerd optimized ISTARSerializer
    debug ISTARSMd         optimized ISTARSM
    ACSCL_x64
    Qt5::Core Qt5::Network
)

# ============================================================
#  运行目录（build/<Config>/）自包含 + exe 旁的配置
#  + 部署形状 output/exe/（给顶层装配，不含配置）
#  平台基础库自带一份 Qt5*.dll + platforms/，所以不需要 windeployqt
# ============================================================
istar_deploy_runtime(${PROJECT_NAME}
    TARGETS io
    DLLS
        "${ACS_LIB_DIR}/ACSCL_x64.dll"
        "${ACS_LIB_DIR}/WibuCm64.dll")

# 默认配置：按"配置根"形状铺到 exe 旁（build\<Config>\ 就是调试时的配置根）
#   mw_config.yaml            根级共享文件
#   io_config\config.json     本模块配置；其中的 acs/、plc/ 以该文件所在目录为基准
istar_stage(${PROJECT_NAME} SRC "${CMAKE_SOURCE_DIR}/config/config.json" TO io_config)
istar_stage(${PROJECT_NAME} SRC "${CMAKE_SOURCE_DIR}/config/mw_config.yaml")
istar_stage(${PROJECT_NAME} SRC "${CMAKE_SOURCE_DIR}/config/acs/io_config.json" TO io_config/acs)
istar_stage(${PROJECT_NAME} SRC "${CMAKE_SOURCE_DIR}/config/plc/io_config.json" TO io_config/plc)
# 外层覆盖（独立构建时不存在 → 跳过）
if(OUTER_CONFIG_DIR)
    istar_stage(${PROJECT_NAME} SRC "${OUTER_CONFIG_DIR}/mw_config.yaml"          OPTIONAL)
    istar_stage(${PROJECT_NAME} SRC "${OUTER_CONFIG_DIR}/io_config/config.json"  OPTIONAL TO io_config)
    istar_stage(${PROJECT_NAME} SRC "${OUTER_CONFIG_DIR}/io_config/acs/io_config.json" OPTIONAL TO io_config/acs)
    istar_stage(${PROJECT_NAME} SRC "${OUTER_CONFIG_DIR}/io_config/plc/io_config.json" OPTIONAL TO io_config/plc)
endif()

istar_install_app(${PROJECT_NAME} APP io DIR exe TARGETS io QT)
