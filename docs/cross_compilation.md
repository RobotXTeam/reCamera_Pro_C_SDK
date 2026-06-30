# Cross-Compilation Guide for RV1126B

This document covers the complete cross-compilation workflow: toolchain
installation, sysroot setup, building the SDK and its examples, and
deploying artifacts to the target device.

---

## 1. Toolchain

### Location

The toolchain is bundled with the platform SDK:

```
/path/to/your/workspace/
  linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/
```

Key binaries (all prefixed `aarch64-rockchip1240-linux-gnu-`):

| Binary | Purpose |
|---|---|
| `gcc` | C compiler |
| `g++` | C++ compiler |
| `ar` | Static library archiver |
| `strip` | Symbol stripper |
| `objdump` | Object file inspector |

### Add to PATH

```bash
export TOOLCHAIN_ROOT="/path/to/your/workspace/\
linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main"
export PATH="${TOOLCHAIN_ROOT}/bin:${PATH}"
```

Verify:

```bash
aarch64-rockchip1240-linux-gnu-g++ --version
# aarch64-rockchip1240-linux-gnu-g++ (GCC ...) ...
```

---

## 2. Sysroot

The sysroot contains device-side headers and shared libraries required
for linking. It lives inside the SDK tree:

```
/path/to/your/workspace/rv1126b_sdk/sysroot/
├── usr/
│   ├── include/          # Platform headers (Rockchip RGA, MPP, RKNN, V4L2, ...)
│   └── lib/              # Import libraries for linking (.so stubs)
└── lib/                  # libc, libpthread, etc.
```

The CMake toolchain file sets `CMAKE_SYSROOT` to this directory, so the
compiler and linker resolve headers and libraries here automatically.

---

## 3. CMake Toolchain File

The file `cmake/toolchain-aarch64.cmake` contains:

```cmake
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_PREFIX "aarch64-rockchip1240-linux-gnu")
set(TOOLCHAIN_ROOT
    "/path/to/your/workspace/\
linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main")

set(CMAKE_C_COMPILER   "${TOOLCHAIN_ROOT}/bin/${TOOLCHAIN_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_ROOT}/bin/${TOOLCHAIN_PREFIX}-g++")
set(CMAKE_AR           "${TOOLCHAIN_ROOT}/bin/${TOOLCHAIN_PREFIX}-ar")
set(CMAKE_STRIP        "${TOOLCHAIN_ROOT}/bin/${TOOLCHAIN_PREFIX}-strip")

set(CMAKE_SYSROOT
    "/path/to/your/workspace/rv1126b_sdk/sysroot")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

Pass it to CMake with `-DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-aarch64.cmake`.

---

## 4. Building the Full SDK

```bash
cd /path/to/your/workspace/rv1126b_sdk

# Release build with all examples
mkdir -p build-aarch64 && cd build-aarch64

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=OFF \
  -GNinja

ninja -j$(nproc)
```

For a debug build (includes symbols, disables optimizations):

```bash
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_EXAMPLES=ON \
  -GNinja
```

---

## 5. Building a Single Example

After the initial CMake configure step, build just one target:

```bash
cd build-aarch64
ninja hello_world
ninja rknn_yolo
```

Or using the CMake `--build` interface with a target filter:

```bash
cmake --build build-aarch64 --target rtsp_server -j4
```

---

## 6. Verifying the Binary Architecture

```bash
file build-aarch64/examples/hello_world
# build-aarch64/examples/hello_world: ELF 64-bit LSB executable, ARM aarch64, ...

aarch64-rockchip1240-linux-gnu-readelf -d build-aarch64/examples/hello_world \
  | grep NEEDED
# 0x0000000000000001 (NEEDED) Shared library: [librv1126b_sdk.so]
# 0x0000000000000001 (NEEDED) Shared library: [libstdc++.so.6]
# ...
```

---

## 7. Stripping Debug Symbols (Release Deployment)

```bash
aarch64-rockchip1240-linux-gnu-strip \
  --strip-unneeded \
  build-aarch64/examples/hello_world
```

---

## 8. Deploying to Device

### Via SCP

```bash
scp build-aarch64/examples/hello_world root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx chmod +x /tmp/hello_world
```

### Via the deploy script

```bash
./tools/deploy.sh build-aarch64/examples/hello_world
# Default target IP: 192.168.x.xx
# Default password: recamera.1

./tools/deploy.sh build-aarch64/examples/rknn_yolo 192.168.x.xx
```

### Deploy the SDK shared library

If `librv1126b_sdk.so` is not already on the device under `/oem/usr/lib/`
or `/usr/lib/`, copy it:

```bash
scp build-aarch64/lib/librv1126b_sdk.so root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx \
  "cp /tmp/librv1126b_sdk.so /oem/usr/lib/ && ldconfig"
```

---

## 9. Common Errors

| Error | Cause | Fix |
|---|---|---|
| `cannot find -lrockchip_mpp` | MPP import lib missing in sysroot | Check `sysroot/usr/lib/librocckchip_mpp.so` exists |
| `error: unrecognized command line option '-march=armv8-a'` | Wrong compiler prefix | Ensure `aarch64-rockchip1240-*` tools are on PATH |
| `RKNN model load failed` on device | Missing `librknn_api.so` | Ensure firmware provides it under `/oem/usr/lib/` |
| `Segfault` on device only | ABI mismatch between sysroot and device libc | Rebuild with matching toolchain/sysroot version |
| `RTSP: bind port 554 failed` | Port 554 requires root | Run as root or use port >= 1024 |
