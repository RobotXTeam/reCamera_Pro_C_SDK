cmake_minimum_required(VERSION 3.16)

project(reCamera_Pro_SDK
    VERSION 1.0.0
    DESCRIPTION "RV1126B Enterprise C++ SDK"
    LANGUAGES C CXX
)

# ─── C++ standard ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# ─── Module path ───────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ─── Build options ─────────────────────────────────────────────────────────────
option(RV1126B_BUILD_SHARED     "Build shared library (.so)"       ON)
option(RV1126B_BUILD_STATIC     "Build static library (.a)"        ON)
option(RV1126B_BUILD_EXAMPLES   "Build example programs"           ON)
option(RV1126B_BUILD_TESTS      "Build unit/integration tests"     OFF)
option(RV1126B_ENABLE_CAMERA    "Enable Camera capture module"     ON)
option(RV1126B_ENABLE_ISP       "Enable ISP module"                ON)
option(RV1126B_ENABLE_NPU       "Enable NPU/RKNN module"           ON)
option(RV1126B_ENABLE_VIDEO     "Enable video encode/decode"       ON)
option(RV1126B_ENABLE_STREAM    "Enable RTSP/RTMP/ONVIF streaming" ON)
option(RV1126B_ENABLE_PERIPH    "Enable GPIO/UART/I2C/SPI/PWM/ADC" ON)
option(RV1126B_ENABLE_IMU       "Enable IMU sensor"                ON)
option(RV1126B_ENABLE_AUDIO     "Enable audio input/output"        ON)

# ─── Install paths ──────────────────────────────────────────────────────────────
include(GNUInstallDirs)
set(RV1126B_INSTALL_LIBDIR  "${CMAKE_INSTALL_LIBDIR}")
set(RV1126B_INSTALL_INCDIR  "${CMAKE_INSTALL_INCLUDEDIR}/rv1126b")
set(RV1126B_INSTALL_BINDIR  "${CMAKE_INSTALL_BINDIR}")

# ─── Version configuration ──────────────────────────────────────────────────────
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/rv1126b/version.h"
    @ONLY
)

# ─── Dependency discovery ────────────────────────────────────────────────────────
find_package(Threads REQUIRED)

if(RV1126B_ENABLE_NPU)
    find_package(RKNNRuntime QUIET)
endif()

if(RV1126B_ENABLE_VIDEO OR RV1126B_ENABLE_STREAM)
    find_package(MPP QUIET)
    find_package(RGA QUIET)
endif()

if(RV1126B_ENABLE_ISP)
    find_package(RKAIQRuntime QUIET)
endif()

if(RV1126B_ENABLE_STREAM)
    find_package(LiveMedia QUIET)
    find_package(RTMP QUIET)
endif()

# ─── Core library (Pre-compiled) ───────────────────────────────────────────────────
add_library(reCamera_pro_sdk SHARED IMPORTED)
set_target_properties(reCamera_pro_sdk PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libreCamera_pro_sdk.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# ─── Example programs ────────────────────────────────────────────────────────────
if(RV1126B_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ─── Tests ──────────────────────────────────────────────────────────────────────
if(RV1126B_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ─── Install targets ────────────────────────────────────────────────────────────
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/rv1126b"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/include/rv1126b/version.h"
    DESTINATION "${RV1126B_INSTALL_INCDIR}"
)

# ─── Export CMake package config ────────────────────────────────────────────────
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/reCamera_Pro_SDK-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/reCamera_Pro_SDK-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/reCamera_Pro_SDK"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/reCamera_Pro_SDK-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/reCamera_Pro_SDK-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/reCamera_Pro_SDK-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/reCamera_Pro_SDK"
)

# ─── Print configuration summary ────────────────────────────────────────────────
message(STATUS "")
message(STATUS "=== RV1126B SDK ${PROJECT_VERSION} ===")
message(STATUS "  Shared lib   : ${RV1126B_BUILD_SHARED}")
message(STATUS "  Static lib   : ${RV1126B_BUILD_STATIC}")
message(STATUS "  Examples     : ${RV1126B_BUILD_EXAMPLES}")
message(STATUS "  Tests        : ${RV1126B_BUILD_TESTS}")
message(STATUS "  NPU module   : ${RV1126B_ENABLE_NPU}")
message(STATUS "  ISP module   : ${RV1126B_ENABLE_ISP}")
message(STATUS "  Video module : ${RV1126B_ENABLE_VIDEO}")
message(STATUS "  Stream module: ${RV1126B_ENABLE_STREAM}")
message(STATUS "  Peripheral   : ${RV1126B_ENABLE_PERIPH}")
message(STATUS "  IMU module   : ${RV1126B_ENABLE_IMU}")
message(STATUS "  Audio module : ${RV1126B_ENABLE_AUDIO}")
message(STATUS "")
