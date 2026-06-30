[English](README.md) | [中文](README_zh.md)

![Platform: RV1126B](https://img.shields.io/badge/platform-RV1126B-blue)
![Language: C++17](https://img.shields.io/badge/language-C%2B%2B17-brightgreen)
![License: Custom/Commercial](https://img.shields.io/badge/license-Custom%2FCommercial-orange)

# reCamera Pro C++ SDK

![reCamera Pro](images/reCamera%20Pro.jpg)

An enterprise-grade C++ SDK for the **RV1126B SoC** used in reCamera devices. It provides a clean, unified C++17 API for camera capture, ISP tuning, NPU inference, GPIO, UART, I2C, IMU, RTSP, RTMP, and ONVIF — all on a single embedded Linux platform (Buildroot 2023.02.6, Linux 6.1.157, aarch64).

---

## Features

| Module | Capability |
|--------|-----------|
| Camera | V4L2 Multiplanar capture (/dev/video13), H.264/MJPEG, frame callbacks |
| ISP | Exposure, white balance, color, noise reduction, HDR (librkaiq.so) |
| NPU | RKNN runtime inference via dlopen, model load/unload, tensor I/O |
| Video | H.264 hardware encoder (MPP), raw Annex-B file recorder |
| RTSP | Embedded RTSP server on port 8554 |
| RTMP | RTMP push client with auto-reconnect |
| ONVIF | ONVIF Profile S discovery + media service on port 8899 |
| GPIO | Export/unexport, direction, read/write, bank number helper |
| UART | Open/close, configurable baud/parity/stop bits, non-blocking read |
| I2C | Register read/write for peripheral devices |
| SPI | SPI bus access |
| PWM | Hardware PWM channel control |
| ADC | Analog-to-digital channel read |
| IMU | ICM-42670 via Linux IIO subsystem, accel + gyro + temperature |
| SysInfo | Kernel version, chip name, NPU core count, CPU temperature |
| Watchdog | Hardware watchdog feed |
| Network | Network interface configuration |
| Logger | Thread-safe, level-filtered logger with optional file sink |

---

## Quick Start

```bash
# 1. Clone
git clone https://github.com/your-org/reCamera_Pro_C++_SDK.git
cd reCamera_Pro_C++_SDK

# 2. Download and extract the toolchain from GitHub Releases (see Compilation Guide)
mkdir -p toolchain
tar -xzf reCamera_Pro_SDK_toolchain_aarch64_vX.X.X.tar.gz -C toolchain/
export RV1126B_TOOLCHAIN_DIR=$(pwd)/toolchain/linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/bin

# 3. Build all examples
./scripts/build.sh release --examples
```

Deploy and run `hello_world` on the device:

```bash
# Deploy SDK library (first time only)
scp build-aarch64/src/libreCamera_pro_sdk.so.1.0.0 root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"

# Deploy and run the demo
scp build-aarch64/examples/hello_world/hello_world root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/hello_world"
```

Expected output:

```
========================================
  reCamera Pro SDK  v1.0.0
========================================
Device       : reCamera
Chip         :
Kernel       : 6.1.157
NPU cores    : 1
CPU temp     : 47.5 °C
Hello, reCamera Pro SDK!
```

---

## Prerequisites

| Requirement | Version / Notes |
|-------------|-----------------|
| Host OS | Ubuntu 20.04+ or Debian-based Linux (x86_64) |
| CMake | 3.16 or newer |
| sshpass | `sudo apt install sshpass` (used by deploy scripts) |
| aarch64 cross-compiler | From GitHub Releases (recommended), or `sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu` |
| Python 3 | Optional, for helper scripts |

---

## Compilation Guide

### Step 1 — Download the toolchain from GitHub Releases

The cross-compiler (~600 MB) is not included in the repository. Download the appropriate archive from the project's GitHub Releases page:

| Asset | Description |
|-------|-------------|
| `reCamera_Pro_SDK_toolchain_aarch64_vX.X.X.tar.gz` | aarch64 cross-compiler — GCC 12.4 (required) |
| `reCamera_Pro_SDK_toolchain_arm_vX.X.X.tar.gz` | ARM 32-bit cross-compiler (optional) |
| `reCamera_Pro_SDK_docs_vX.X.X.tar.gz` | RV1126B hardware documentation |

Extract and configure:

```bash
mkdir -p toolchain
tar -xzf reCamera_Pro_SDK_toolchain_aarch64_vX.X.X.tar.gz -C toolchain/

# The extracted directory name contains the version string. Adjust accordingly.
export RV1126B_TOOLCHAIN_DIR=$(pwd)/toolchain/linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/bin

# Verify the compiler is accessible
${RV1126B_TOOLCHAIN_DIR}/aarch64-rockchip1240-linux-gnu-g++ --version
```

### Step 2 — Build with the provided script

```bash
# Release build with all 12 examples
./scripts/build.sh release --examples

# Debug build
./scripts/build.sh debug --examples

# Library only (no examples)
./scripts/build.sh release
```

Build artifacts are written to `build-aarch64/`.

### Step 3 — Manual CMake build

If you prefer to invoke CMake directly:

```bash
cmake -B build-aarch64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=ON
cmake --build build-aarch64 -j$(nproc)
```

### Step 4 — Alternative: system cross-compiler

The Debian/Ubuntu system compiler also works:

```bash
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake -B build-aarch64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=ON
cmake --build build-aarch64 -j$(nproc)
```

The system compiler targets a newer ABI than the Rockchip toolchain; binary compatibility with the device's system libraries is not guaranteed for all modules.

---

## Project Directory Structure

```
reCamera_Pro_C++_SDK/
├── CMakeLists.txt              Top-level CMake (C++17, 14 build options)
├── cmake/
│   ├── toolchain-aarch64.cmake Primary aarch64 cross-compilation toolchain
│   ├── toolchain-arm.cmake     32-bit ARM toolchain (alternate)
│   ├── FindMPP.cmake           Rockchip MPP (hardware video codec) finder
│   ├── FindRKNNRuntime.cmake   RKNN Runtime finder
│   ├── FindRGA.cmake           RGA 2D hardware accelerator finder
│   └── FindRKAIQRuntime.cmake  ISP AIQ (rkaiq) finder
├── include/rv1126b/            Public API headers
│   ├── core/                   error.hpp, logger.hpp, config.hpp, utils.hpp
│   ├── camera/                 camera.hpp
│   ├── isp/                    isp.hpp
│   ├── npu/                    npu.hpp, tensor.hpp
│   ├── video/                  encoder.hpp, recorder.hpp
│   ├── stream/                 rtsp.hpp, rtmp.hpp, onvif.hpp
│   ├── peripheral/             gpio.hpp, uart.hpp, i2c.hpp, spi.hpp, pwm.hpp, adc.hpp
│   ├── sensor/                 imu.hpp
│   ├── system/                 sysinfo.hpp, watchdog.hpp
│   └── network/                netconfig.hpp
├── lib/                        Precompiled SDK binary black-box library (libreCamera_pro_sdk.so)
├── examples/                   12 standalone demo programs
│   ├── hello_world/            SDK version + system info
│   ├── gpio_control/           LED blink + input poll
│   ├── uart_echo/              Serial echo on /dev/ttyS2
│   ├── imu_read/               50 Hz IMU sampling
│   ├── camera_preview/         1080p H.264 preview at 30 fps
│   ├── capture_image/          JPEG + NV12 snapshot
│   ├── record_video/           Raw H.264 file recorder
│   ├── rknn_yolo/              YOLOX-S NPU object detection
│   ├── isp_control/            ISP parameter tuning
│   ├── rtsp_server/            RTSP streaming server
│   ├── rtmp_push/              RTMP push client
│   └── onvif_server/           ONVIF Profile S server
├── sysroot/                    Prebuilt Rockchip libraries for host-side linking
├── extracted/                  Rockchip platform reference code (read-only)
├── scripts/
│   └── build.sh                One-command cross-compilation script
├── tools/
│   ├── deploy.sh               Deploy binary to device via SCP
│   └── device_info.sh          Query device hardware info
└── tests/                      Unit / integration tests (placeholder)
```

---

## Deploying the Application

### Deploy the SDK shared library (required once per device)

```bash
scp build-aarch64/src/libreCamera_pro_sdk.so.1.0.0 root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"
```

### Deploy a demo binary

```bash
scp build-aarch64/examples/hello_world/hello_world root@192.168.x.xx:/tmp/
```

The `tools/deploy.sh` helper combines both steps:

```bash
./tools/deploy.sh build-aarch64/examples/hello_world/hello_world 192.168.x.xx
```

Default device credentials: IP `192.168.x.xx`, user `root`, password `recamera.1`.

---

## Running on Device

```bash
ssh root@192.168.x.xx
LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/hello_world
```

The `LD_LIBRARY_PATH` prefix allows the dynamic linker to find:
- SDK library (`libreCamera_pro_sdk.so.1`) in `/tmp`
- Rockchip platform libraries (`librknnrt.so`, `librkaiq.so`, `librga.so`, `librockchip_mpp.so`) in `/oem/usr/lib`

### Managing rkipc

Most camera demos require the system camera process (`rkipc`) to be stopped first because it holds `/dev/video13`:

```bash
# Stop before running camera demos
/oem/usr/bin/RkLunch-stop.sh

# Restart after testing
/oem/usr/bin/RkLunch.sh
```

The `isp_control` demo is the exception — it needs rkipc **running** to access the rkaiq context.

---

## Available Examples

| Example | Description | rkipc state |
|---------|-------------|------------|
| [hello_world](examples/hello_world/) | SDK version, device info, CPU temperature | either |
| [gpio_control](examples/gpio_control/) | GPIO output (LED blink) and input polling | either |
| [uart_echo](examples/uart_echo/) | UART serial echo on /dev/ttyS2 | either |
| [imu_read](examples/imu_read/) | 50 Hz IMU sampling: accel, gyro, temperature | either |
| [camera_preview](examples/camera_preview/) | 1920x1080 H.264 preview at 30 fps | stopped |
| [capture_image](examples/capture_image/) | JPEG + NV12 single-frame snapshot | stopped |
| [record_video](examples/record_video/) | Raw H.264 Annex-B recording to file | stopped |
| [rknn_yolo](examples/rknn_yolo/) | YOLOX-S object detection on NPU (~14 FPS) | stopped |
| [isp_control](examples/isp_control/) | Exposure, WB, color, NR, HDR tuning | running |
| [rtsp_server](examples/rtsp_server/) | RTSP streaming server on port 8554 | stopped |
| [rtmp_push](examples/rtmp_push/) | RTMP push to external server | stopped |
| [onvif_server](examples/onvif_server/) | ONVIF Profile S discovery server on port 8899 | stopped |

---

## Adding New Demos

### Step 1 — Create the demo directory

```bash
mkdir examples/my_demo
```

### Step 2 — Write main.cpp

```cpp
// examples/my_demo/main.cpp
#include <rv1126b/camera.hpp>
#include <rv1126b/gpio.hpp>
#include <cstdio>

int main() {
    rv1126b::GPIO led(78);  // GPIO2_PB6
    led.open(rv1126b::GPIO::Direction::Output);
    led.set(1);
    led.close();
    return 0;
}
```

### Step 3 — Add CMakeLists.txt

```cmake
# examples/my_demo/CMakeLists.txt
add_executable(my_demo main.cpp)
target_link_libraries(my_demo PRIVATE rv1126b_sdk::rv1126b_sdk)
```

### Step 4 — Register in examples/CMakeLists.txt

```cmake
add_subdirectory(my_demo)
```

### Step 5 — Bring in a new library (if needed)

**Case A — Library already on device (e.g. libasound):**

```cmake
# cmake/FindAlsa.cmake
find_library(ALSA_LIBRARY asound HINTS "${CMAKE_SYSROOT}/usr/lib")
find_path(ALSA_INCLUDE_DIR alsa/asoundlib.h HINTS "${CMAKE_SYSROOT}/usr/include")
```

Add to `src/CMakeLists.txt`:

```cmake
target_link_libraries(reCamera_pro_sdk_shared PRIVATE ${ALSA_LIBRARY})
```

**Case B — New library not yet on device:**

1. Copy `.so` to `sysroot/oem/usr/lib/` for host-side linking
2. Copy headers to `sysroot/oem/usr/include/yourlib/`
3. Copy `.so` to device: `scp yourlib.so root@192.168.x.xx:/tmp/`
4. Write `cmake/FindYourLib.cmake`

**Case C — Header-only library (e.g. nlohmann/json):**

```bash
cp -r nlohmann third_party/
```

```cmake
target_include_directories(reCamera_pro_sdk_shared PRIVATE ${CMAKE_SOURCE_DIR}/third_party)
```

### Step 6 — Build and deploy

```bash
./scripts/build.sh release --examples
./tools/deploy.sh build-aarch64/examples/my_demo/my_demo 192.168.x.xx
```

---

## API Overview

All API calls return a `Result<T>`. Check `.ok()` before dereferencing with `*result`. Errors are readable via `rv1126b::to_string(result.error())`.

| Header | Class | Key Methods |
|--------|-------|-------------|
| `rv1126b/camera.hpp` | `rv1126b::Camera` | `open()`, `start()`, `stop()`, `set_frame_callback()`, `capture_jpeg()` |
| `rv1126b/isp.hpp` | `rv1126b::ISP` | `init()`, `set_exposure()`, `set_white_balance()`, `set_color()`, `set_noise_reduction()`, `set_hdr_mode()` |
| `rv1126b/npu.hpp` | `rv1126b::NPU` | `init()`, `load_model()`, `infer()`, `release_outputs()`, `unload_model()`, `deinit()` |
| `rv1126b/gpio.hpp` | `rv1126b::GPIO` | `open()`, `close()`, `set()`, `get()` |
| `rv1126b/uart.hpp` | `rv1126b::UART` | `open()`, `close()`, `read()`, `write()` |
| `rv1126b/i2c.hpp` | `rv1126b::I2C` | `open()`, `close()`, `read_reg()`, `write_reg()` |
| `rv1126b/imu.hpp` | `rv1126b::IMU` | `init()`, `read()` |
| `rv1126b/rtsp_server.hpp` | `rv1126b::RTSPServer` | `start()`, `stop()`, `push_frame()`, `client_count()` |
| `rv1126b/rtmp_pusher.hpp` | `rv1126b::RTMPPusher` | `connect()`, `disconnect()`, `push_frame()` |
| `rv1126b/onvif_server.hpp` | `rv1126b::ONVIFServer` | `start()`, `stop()` |
| `rv1126b/sysinfo.hpp` | `rv1126b::sys` | `get_info()` |
| `rv1126b/logger.hpp` | `rv1126b::Logger` | `get()`, `set_level()`, `enable_file()` |
| `rv1126b/watchdog.hpp` | `rv1126b::Watchdog` | `open()`, `feed()`, `close()` |

---

## Known Limitations

- **ISP control requires rkipc**: The ISP module wraps `librkaiq.so`, but the full pipeline is owned by `rkipc`. Setters return an error when called without an active rkipc-provided rkaiq context.
- **Camera and rkipc conflict**: The camera module opens `/dev/video13` (ISP output, Multiplanar NV12). This conflicts with rkipc which holds the same node. Stop rkipc before running any camera demo.
- **RTSP port 8554**: Port 554 is occupied by `go2rtc` on the stock firmware. The SDK defaults to 8554.
- **RKNN model path**: The YOLO demo expects `/oem/usr/share/model/rknn/yolox_s.rknn`. Copy the model there if missing.
- **IQ files required for ISP init**: `isp.init()` expects IQ tuning files in `/oem/usr/share/iqfiles`.
- **GPIO pin numbers**: Available pins depend on the board variant. Verify with `cat /sys/kernel/debug/gpio` on the device before hardcoding pin numbers.
- **NV12 direct output**: The `capture_image` demo requests NV12 frames via the frame callback, but NV12 availability depends on the encoder pipeline configuration at runtime.

---

## License

Copyright (c) 2026 RV1126B SDK Project. All Rights Reserved.

**Dual License Policy:**

1. **Non-Commercial / Educational Use:**
   This SDK is open for non-commercial, personal, research, and educational purposes. You may freely use, modify, and distribute the code under these conditions.

2. **Commercial Use:**
   Commercial use (including but not limited to integrating this SDK into a commercial product, using it in a revenue-generating service, or distributing it for profit) is **strictly prohibited without a valid commercial license**. 
   
For commercial licensing inquiries, please contact the repository owner or the reCamera Pro team.
