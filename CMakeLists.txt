cmake_minimum_required(VERSION 3.26)
project(IO VERSION 1.0.0.0)

# ---- 共享构建模块在本工程内（module/io/cmake/），独立构建不依赖任何外层工程 ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(istar_common)    # BASIC_LIB / BASIC_INC / BASIC_CONFIG / ISTAR_GENERATOR
                         # OUTER_CONFIG_DIR / ISTAR_OUTPUT_DIR / istar_runtime_shared / istar_add_app()
include(istar_runtime)   # istar_deploy_runtime() / istar_deploy_first() / istar_stage()
include(istar_install)   # istar_install_app() / istar_install_interface() / istar_finalize_deploy()
include(istar_rpc)       # istar_gen_rpc()

if(NOT DEFINED BASIC_LIB OR NOT DEFINED BASIC_INC)
    message(FATAL_ERROR "istar_common.cmake 未生效：请检查 CMAKE_MODULE_PATH")
endif()

add_subdirectory(interface)
add_subdirectory(MainProject)
add_subdirectory(GUI)

# 把 install 挂进默认构建 —— Qt Creator / VSCode / 命令行"构建一下"都会产出 output/
istar_finalize_deploy()
