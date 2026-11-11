project(io VERSION 1.0.0.0)

# ============================================================
#  io —— 协议库（RPC），被 MainProject / GUI 直接 link target 使用
#
#  与旧做法的区别：
#    - RPC 生成物只落 build/gen/io/，**不回写 interface/include/**
#      （旧做法把 3 个生成头拷回源码树，再用"删它们"驱动重生成）
#    - 不再 POST_BUILD 回写 interface/lib/（下游需要的包由 install 规则产出）
#    - 消费方直接 target_link_libraries(io)，不再依赖 interface/lib 路径
# ============================================================

istar_gen_rpc(io
    API_DIR "${CMAKE_SOURCE_DIR}/interface/include"
    GEN_DIR "${CMAKE_BINARY_DIR}/gen/io"
    IFACE_TARGET io_rpc_iface)

add_library(io SHARED ${ISTAR_RPC_io_SOURCES})

target_compile_definitions(io PUBLIC ISTAR_BUILDING)

# interface/include = 手写的 io_controller.h / ioc_data.h
# build/gen/io/include = 生成的 io_client/_server/_define.h（对消费方可见）
target_include_directories(io PUBLIC
    "${CMAKE_SOURCE_DIR}/interface/include"
    "${CMAKE_BINARY_DIR}/gen/io/include")

target_include_directories(io PRIVATE
    "${BASIC_INC}/common/include"
    "${BASIC_INC}/istarlog/include"
    "${BASIC_INC}/istarmw/include"
    "${BASIC_INC}/istargf/include"
    "${BASIC_INC}/istarthreadpool/include"
    "${BASIC_INC}/istarcsv/include"
    "${BASIC_INC}/istarencryption/include"
    "${BASIC_INC}/istarfilesystem/include"
    "${BASIC_INC}/istarserializer/include"
    "${BASIC_INC}/istarsm/include"
    "${BASIC_INC}/istaryaml/include"
    "${BASIC_INC}/istarzmqworker/include")

target_link_directories(io PUBLIC "${BASIC_LIB}")

target_link_libraries(io PUBLIC
    debug ISTARLogd         optimized ISTARLog
    debug ISTARYamld        optimized ISTARYaml
    debug ISTARMWd          optimized ISTARMW
    debug ISTARGFd          optimized ISTARGF
    debug ISTARThreadPoold  optimized ISTARThreadPool
    debug ISTARCsvd         optimized ISTARCsv
    debug ISTAREncryptiond  optimized ISTAREncryption
    debug ISTARFileSystemd  optimized ISTARFileSystem
    debug ISTARSerializerd  optimized ISTARSerializer
    debug ISTARSMd          optimized ISTARSM
    debug ISTARZmqWorkerd   optimized ISTARZmqWorker)

# ============================================================
#  下游集成包 output/interface/rpc/（给别的产品拿接口用，不进 exe）：
#      output/interface/rpc/include/                头文件
#      output/interface/rpc/lib/{debug,release}/    dll + 导入库
#  output/ 按出口分：exe/ 给顶层装配；interface/<用途>/ 给下游，
#  rpc = 协议完整包（手写头 + 生成头 + 本模块 dll + 导入库）
# ============================================================
istar_install_interface(io SUB rpc FILES
    "${CMAKE_SOURCE_DIR}/interface/include/io_controller.h"
    "${CMAKE_SOURCE_DIR}/interface/include/ioc_data.h"
    "${CMAKE_BINARY_DIR}/gen/io/include/io_client.h"
    "${CMAKE_BINARY_DIR}/gen/io/include/io_server.h"
    "${CMAKE_BINARY_DIR}/gen/io/include/io_define.h")
