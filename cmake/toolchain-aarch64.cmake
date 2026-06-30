# cmake/toolchain-aarch64.cmake
# AArch64 cross-compilation toolchain for reCamera Pro SDK
#
# Usage:
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake ..
#
# Toolchain setup — pick one option:
#
#   Option A (Recommended): Download from GitHub Releases, place alongside SDK:
#     Releases/reCamera_RV1126B_Toolchain_aarch64_vX.X.X.tar.gz
#     Extract so the directory structure is:
#       <workspace>/
#         reCamera_Pro_C++_SDK/      <- this repo
#         reCamera_RV1126B_Toolchain/ <- toolchain (auto-detected)
#     No env var needed if placed at the expected sibling path.
#
#   Option B: Custom path via environment variable:
#     export RV1126B_TOOLCHAIN_DIR=/path/to/toolchain/bin
#
#   Option C: System cross-compiler (Ubuntu/Debian):
#     sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
#     (auto-detected, no configuration needed)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# ── Toolchain binary directory ────────────────────────────────────────────────
get_filename_component(_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
get_filename_component(_WORKSPACE "${_SDK_ROOT}/.." ABSOLUTE)

if(DEFINED ENV{RV1126B_TOOLCHAIN_DIR})
    # Option B: explicit env var
    set(_TOOLCHAIN_BIN "$ENV{RV1126B_TOOLCHAIN_DIR}")
    set(_CROSS_PREFIX  "aarch64-rockchip1240-linux-gnu")
    message(STATUS "Toolchain [env]: ${_TOOLCHAIN_BIN}")

else()
    # Option A: sibling directory (same level as reCamera_Pro_C++_SDK)
    set(_SIBLING_TOOLCHAIN
        "${_WORKSPACE}/reCamera_RV1126B_Toolchain/linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/bin")

    if(EXISTS "${_SIBLING_TOOLCHAIN}")
        set(_TOOLCHAIN_BIN "${_SIBLING_TOOLCHAIN}")
        set(_CROSS_PREFIX  "aarch64-rockchip1240-linux-gnu")
        message(STATUS "Toolchain [sibling]: ${_TOOLCHAIN_BIN}")

    else()
        # Option C: system cross-compiler fallback
        find_program(_SYS_GCC aarch64-linux-gnu-gcc
            PATHS /usr/bin /usr/local/bin NO_DEFAULT_PATH)
        if(_SYS_GCC)
            get_filename_component(_TOOLCHAIN_BIN "${_SYS_GCC}" DIRECTORY)
            set(_CROSS_PREFIX "aarch64-linux-gnu")
            message(STATUS "Toolchain [system]: ${_TOOLCHAIN_BIN}")
        else()
            message(FATAL_ERROR
                "\n[ERROR] AArch64 cross-compiler not found.\n\n"
                "Options:\n"
                "  A) Place toolchain as sibling directory:\n"
                "       ${_WORKSPACE}/reCamera_RV1126B_Toolchain/\n"
                "     Download: https://github.com/your-org/reCamera-Pro-SDK/releases\n\n"
                "  B) Set env var:\n"
                "       export RV1126B_TOOLCHAIN_DIR=/path/to/toolchain/bin\n\n"
                "  C) Install system cross-compiler:\n"
                "       sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu\n")
        endif()
    endif()
endif()

# ── Compiler paths ────────────────────────────────────────────────────────────
set(CMAKE_C_COMPILER   "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-gcc"  CACHE STRING "")
set(CMAKE_CXX_COMPILER "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-g++"  CACHE STRING "")
set(CMAKE_AR           "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-ar"   CACHE STRING "")
set(CMAKE_STRIP        "${_TOOLCHAIN_BIN}/${_CROSS_PREFIX}-strip" CACHE STRING "")

# ── GCC sysroot (standard libc/libstdc++) ─────────────────────────────────────
execute_process(
    COMMAND "${CMAKE_C_COMPILER}" "--print-sysroot"
    OUTPUT_VARIABLE _GCC_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
set(CMAKE_SYSROOT "${_GCC_SYSROOT}")

# ── Platform SDK (Rockchip prebuilt libs + headers) ───────────────────────────
# The sysroot/ directory is bundled with this repo (~12 MB).
# Override with RV1126B_PLATFORM_SDK if you use an external platform package.
if(DEFINED ENV{RV1126B_PLATFORM_SDK})
    set(_PLATFORM_SDK "$ENV{RV1126B_PLATFORM_SDK}")
    message(STATUS "Platform SDK [env]: ${_PLATFORM_SDK}")
else()
    set(_PLATFORM_SDK "${_SDK_ROOT}/sysroot")
    message(STATUS "Platform SDK [bundled]: ${_PLATFORM_SDK}")
endif()

list(APPEND CMAKE_SYSTEM_INCLUDE_PATH
    "${_PLATFORM_SDK}/oem/usr/include"
    "${_PLATFORM_SDK}/oem/usr/include/rknn"
    "${_PLATFORM_SDK}/oem/usr/include/rockchip"
    "${_PLATFORM_SDK}/usr/include"
)
list(APPEND CMAKE_SYSTEM_LIBRARY_PATH
    "${_PLATFORM_SDK}/oem/usr/lib"
    "${_PLATFORM_SDK}/usr/lib"
)
set(CMAKE_FIND_ROOT_PATH    "${CMAKE_SYSROOT}" "${_PLATFORM_SDK}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

add_compile_options(-O2 -pipe -fstack-protector-strong -fPIC)

message(STATUS "------------------------------------------------------------")
message(STATUS " CC      : ${CMAKE_C_COMPILER}")
message(STATUS " SYSROOT : ${CMAKE_SYSROOT}")
message(STATUS " PLATFORM: ${_PLATFORM_SDK}")
message(STATUS "------------------------------------------------------------")
