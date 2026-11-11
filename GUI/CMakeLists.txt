project(GUI_MDL_IO VERSION 1.0.0.0)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

set(GUI_SOURCES
    main.cpp
    MainWindow/MainWindow.cpp
    IoGroup/IoGroup.cpp
    IoWidget/DiWidget.cpp
    IoWidget/DoWidget.cpp
    IoWidget/AiWidget.cpp
    IoWidget/AoWidget.cpp
    IoWidget/SettingWidget.cpp
    IoWidget/BufferWidget.cpp
)

set(GUI_HEADERS
    MainWindow/MainWindow.h
    IoGroup/IoGroup.h
    IoWidget/DiWidget.h
    IoWidget/DoWidget.h
    IoWidget/AiWidget.h
    IoWidget/AoWidget.h
    IoWidget/SettingWidget.h
    IoWidget/BufferWidget.h
    ${CMAKE_SOURCE_DIR}/interface/include/ioc_data.h
)

set(GUI_UIS
    IoWidget/DiWidget.ui
    IoWidget/DoWidget.ui
    IoWidget/AiWidget.ui
    IoWidget/AoWidget.ui
    IoWidget/SettingWidget.ui
    IoWidget/BufferWidget.ui
)

source_group("Source Files" FILES ${GUI_SOURCES})
source_group("Header Files" FILES ${GUI_HEADERS})
source_group("Ui Files" FILES ${GUI_UIS})

istar_add_app(${PROJECT_NAME} SOURCES ${GUI_SOURCES} ${GUI_HEADERS} ${GUI_UIS})

target_compile_definitions(${PROJECT_NAME} PUBLIC ISTAR_BUILDING)

target_include_directories(${PROJECT_NAME} BEFORE PUBLIC
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${BASIC_INC}/common/include
    ${BASIC_INC}/istarlog/include
    ${BASIC_INC}/istarmw/include
    ${BASIC_INC}/istaryaml/include
    ${BASIC_INC}/istargf/include
    ${BASIC_INC}/istarthreadpool/include
    ${BASIC_INC}/istarserializer/include
    ${BASIC_INC}/istarsm/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${BASIC_LIB}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    io                           # 同工程内的协议库，直接 link target
    debug ISTARMWd         optimized ISTARMW
    debug ISTARYamld       optimized ISTARYaml
    debug ISTARLogd        optimized ISTARLog
    debug ISTARGFd         optimized ISTARGF
    debug ISTARThreadPoold optimized ISTARThreadPool
    debug ISTARSerializerd optimized ISTARSerializer
    debug ISTARSMd         optimized ISTARSM
    Qt5::Core Qt5::Widgets
)

# ============================================================
#  运行目录（build/<Config>/）自包含 + exe 旁的配置
#  + 部署形状 output/exe/（给顶层装配，不含配置）
#  平台基础库自带一份 Qt5*.dll + platforms/，所以不需要 windeployqt
# ============================================================
istar_deploy_runtime(${PROJECT_NAME} TARGETS io)

# GUI 默认配置只有 mw_config.yaml（config.json / acs / plc 是控制层的事）
istar_stage(${PROJECT_NAME} SRC "${CMAKE_SOURCE_DIR}/config/mw_config.yaml")
if(OUTER_CONFIG_DIR)
    istar_stage(${PROJECT_NAME} SRC "${OUTER_CONFIG_DIR}/mw_config.yaml" OPTIONAL)
endif()

istar_install_app(${PROJECT_NAME} APP io DIR exe TARGETS io QT)
