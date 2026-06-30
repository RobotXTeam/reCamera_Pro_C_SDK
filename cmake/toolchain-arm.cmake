# cmake/toolchain-arm.cmake
# ARM 32-bit cross-compilation toolchain (arm-rockchip1240-linux-gnueabihf)
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake ..
#
# Set toolchain path:
#   export RV1126B_TOOLCHAIN_DIR=/path/to/arm-toolchain/bin

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(DEFINED ENV{RV1126B_TOOLCHAIN_DIR})
    set(_TOOLCHAIN_BIN "$ENV{RV1126B_TOOLCHAIN_DIR}")
    set(_CROSS_PREFIX  "arm-rockchip1240-linux-gnueabihf")
else()
    find_program(_SYS_GCC arm-linux-gnueabihf-gcc PATHS /usr/bin /usr/local/bin)
    if(_SYS_GCC)
        get_filename_component(_TOOLCHAIN_BIN "${_SYS_GCC}" DIRECTORY)
        set(_CROSS_PREFIX "arm-linux-gnueabihf")
    else()
        message(FATAL_ERROR
            "\n[ERROR] ARM cross-compiler not found.\n"
            "Set RV1126B_TOOLCHAIN_DIR or install:\n"
            "  sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf\n")
    endif()
endif()

set(CMAKE_C_COMPILER   "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-gcc"  CACHE STRING "")
set(CMAKE_CXX_COMPILER "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-g++"  CACHE STRING "")
set(CMAKE_AR           "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-ar"   CACHE STRING "")
set(CMAKE_STRIP        "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-strip" CACHE STRING "")

execute_process(
    COMMAND "${CMAKE_C_COMPILER}" "--print-sysroot"
    OUTPUT_VARIABLE _GCC_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
set(CMAKE_SYSROOT "${_GCC_SYSROOT}")

get_filename_component(_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
if(DEFINED ENV{RV1126B_PLATFORM_SDK})
    set(_PLATFORM_SDK "$ENV{RV1126B_PLATFORM_SDK}")
else()
    set(_PLATFORM_SDK "${_REPO_ROOT}/sysroot")
endif()

list(APPEND CMAKE_SYSTEM_INCLUDE_PATH
    "${_PLATFORM_SDK}/oem/usr/include"
    "${_PLATFORM_SDK}/oem/usr/include/rknn"
    "${_PLATFORM_SDK}/oem/usr/include/rockchip"
)
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
    "${_PLATFORM_SDK}/oem/usr/lib"
    "${_PLATFORM_SDK}/usr/lib"
)

set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}" "${_PLATFORM_SDK}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

add_compile_options(-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -O2 -fPIC)

message(STATUS "ARM toolchain: ${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-gcc")
