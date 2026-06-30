[English](README.md) | [中文](README_zh.md)

![Platform: RV1126B](https://img.shields.io/badge/platform-RV1126B-blue)
![Language: C++17](https://img.shields.io/badge/language-C%2B%2B17-brightgreen)
![License: Custom/Commercial](https://img.shields.io/badge/license-Custom%2FCommercial-orange)

# reCamera Pro C++ SDK

![reCamera Pro](images/reCamera%20Pro.jpg)

这是一款专为 reCamera 设备中使用的 **RV1126B SoC** 打造的企业级 C++ SDK。它提供了清晰、统一的 C++17 API，用于实现摄像头采集、ISP 调节、NPU 推理、GPIO、UART、I2C、IMU、RTSP、RTMP 和 ONVIF 功能——所有这些都集成在一个嵌入式 Linux 平台上（Buildroot 2023.02.6, Linux 6.1.157, aarch64）。

---

## 功能特性 (Features)

| 模块 (Module) | 能力 (Capability) |
|--------|-----------|
| Camera | V4L2 多平面采集 (/dev/video13)、H.264/MJPEG 编码、帧回调 |
| ISP | 曝光、白平衡、色彩、降噪、HDR (librkaiq.so) |
| NPU | 通过 dlopen 实现 RKNN 运行时推理、模型加载/卸载、张量 I/O |
| Video | H.264 硬件编码器 (MPP)、原生 Annex-B 文件录制 |
| RTSP | 内置的 RTSP 服务器，运行于 8554 端口 |
| RTMP | 带有自动重连功能的 RTMP 推流客户端 |
| ONVIF | ONVIF Profile S 发现功能 + 媒体服务，运行于 8899 端口 |
| GPIO | 导出/取消导出、方向设置、读/写操作、Bank 编号辅助工具 |
| UART | 打开/关闭、可配置波特率/奇偶校验/停止位、非阻塞读取 |
| I2C | 外围设备的寄存器读/写操作 |
| SPI | SPI 总线访问 |
| PWM | 硬件 PWM 通道控制 |
| ADC | 模数转换 (ADC) 通道读取 |
| IMU | 通过 Linux IIO 子系统支持 ICM-42670，提供加速度、陀螺仪及温度数据 |
| SysInfo | 内核版本、芯片名称、NPU 核心数、CPU 温度 |
| Watchdog | 硬件看门狗喂狗操作 |
| Network | 网络接口配置 |
| Logger | 线程安全、支持日志级别过滤的记录器，可选输出至文件 |

---

## 快速入门 (Quick Start)

```bash
# 1. 克隆仓库
git clone https://github.com/your-org/reCamera_Pro_C++_SDK.git
cd reCamera_Pro_C++_SDK

# 2. 从 GitHub Releases 下载并解压交叉编译工具链（详见“编译指南”）
mkdir -p toolchain
tar -xzf reCamera_Pro_SDK_toolchain_aarch64_vX.X.X.tar.gz -C toolchain/
export RV1126B_TOOLCHAIN_DIR=$(pwd)/toolchain/linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/bin

# 3. 编译所有示例代码
./scripts/build.sh release --examples
```

在设备上部署并运行 `hello_world`：

```bash
# 部署 SDK 动态链接库（仅首次需要）
scp build-aarch64/src/libreCamera_pro_sdk.so.1.0.0 root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"

# 部署并运行演示程序
scp build-aarch64/examples/hello_world/hello_world root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/hello_world"
```

预期输出：

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

## 前置要求 (Prerequisites)

| 要求 (Requirement) | 版本/说明 (Version / Notes) |
|-------------|-----------------|
| 宿主机操作系统 (Host OS) | Ubuntu 20.04+ 或基于 Debian 的 Linux 系统 (x86_64) |
| CMake | 3.16 或更新版本 |
| sshpass | `sudo apt install sshpass` （部署脚本需要用到） |
| aarch64 交叉编译器 | 推荐从 GitHub Releases 下载，或使用 `sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu` 安装 |
| Python 3 | 可选，用于运行辅助脚本 |

---

## 编译指南 (Compilation Guide)

### 第 1 步 — 从 GitHub Releases 下载工具链

交叉编译器（大约 600 MB）并未包含在代码仓库中。请从项目的 GitHub Releases 页面下载对应的压缩包：

| 文件 (Asset) | 说明 (Description) |
|-------|-------------|
| `reCamera_Pro_SDK_toolchain_aarch64_vX.X.X.tar.gz` | aarch64 交叉编译器 — GCC 12.4 （必须） |
| `reCamera_Pro_SDK_toolchain_arm_vX.X.X.tar.gz` | ARM 32 位交叉编译器 （可选） |
| `reCamera_Pro_SDK_docs_vX.X.X.tar.gz` | RV1126B 硬件文档 |

解压并配置环境变量：

```bash
mkdir -p toolchain
tar -xzf reCamera_Pro_SDK_toolchain_aarch64_vX.X.X.tar.gz -C toolchain/

# 解压后的目录名称包含版本号。请根据实际情况进行调整。
export RV1126B_TOOLCHAIN_DIR=$(pwd)/toolchain/linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/bin

# 验证编译器是否可用
${RV1126B_TOOLCHAIN_DIR}/aarch64-rockchip1240-linux-gnu-g++ --version
```

### 第 2 步 — 使用提供的脚本进行编译

```bash
# 构建 Release 版本，包含所有 12 个示例
./scripts/build.sh release --examples

# 构建 Debug 版本
./scripts/build.sh debug --examples

# 仅构建库文件（不包含示例）
./scripts/build.sh release
```

编译产物将输出至 `build-aarch64/` 目录。

### 第 3 步 — 手动使用 CMake 编译

如果您更倾向于直接调用 CMake：

```bash
cmake -B build-aarch64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=ON
cmake --build build-aarch64 -j$(nproc)
```

### 第 4 步 — 备选方案：使用系统级交叉编译器

使用 Debian/Ubuntu 系统自带的编译器同样可行：

```bash
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
cmake -B build-aarch64 \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=ON
cmake --build build-aarch64 -j$(nproc)
```

系统编译器的目标 ABI 可能比 Rockchip 工具链的更新；因此，不能保证所有模块与设备系统库的二进制兼容性。

---

## 项目目录结构 (Project Directory Structure)

```
reCamera_Pro_C++_SDK/
├── CMakeLists.txt              顶层 CMake 配置 (C++17, 14 个构建选项)
├── cmake/
│   ├── toolchain-aarch64.cmake 主要的 aarch64 交叉编译工具链
│   ├── toolchain-arm.cmake     32 位 ARM 工具链 (备选)
│   ├── FindMPP.cmake           Rockchip MPP (硬件视频编解码器) 查找模块
│   ├── FindRKNNRuntime.cmake   RKNN 运行时查找模块
│   ├── FindRGA.cmake           RGA 2D 硬件加速器查找模块
│   └── FindRKAIQRuntime.cmake  ISP AIQ (rkaiq) 查找模块
├── include/rv1126b/            公共 API 头文件
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
├── src/                        SDK 库的具体实现 (~25 个 .cpp 文件)
├── examples/                   12 个独立的演示程序
│   ├── hello_world/            SDK 版本信息 + 系统信息
│   ├── gpio_control/           LED 闪烁 + 输入轮询
│   ├── uart_echo/              在 /dev/ttyS2 上的串口回显
│   ├── imu_read/               50 Hz IMU 采样
│   ├── camera_preview/         30 fps 的 1080p H.264 预览
│   ├── capture_image/          JPEG + NV12 快照抓取
│   ├── record_video/           原生 H.264 文件录制
│   ├── rknn_yolo/              YOLOX-S NPU 目标检测
│   ├── isp_control/            ISP 参数调节
│   ├── rtsp_server/            RTSP 流媒体服务器
│   ├── rtmp_push/              RTMP 推流客户端
│   └── onvif_server/           ONVIF Profile S 服务器
├── sysroot/                    预编译的 Rockchip 库，用于宿主机端链接
├── extracted/                  Rockchip 平台参考代码 (只读)
├── scripts/
│   └── build.sh                一键式交叉编译脚本
├── tools/
│   ├── deploy.sh               通过 SCP 将二进制文件部署到设备的工具
│   └── device_info.sh          查询设备硬件信息的脚本
└── tests/                      单元测试 / 集成测试 (占位符)
```

---

## 部署应用程序 (Deploying the Application)

### 部署 SDK 动态链接库（每台设备仅需执行一次）

```bash
scp build-aarch64/src/libreCamera_pro_sdk.so.1.0.0 root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"
```

### 部署演示程序

```bash
scp build-aarch64/examples/hello_world/hello_world root@192.168.x.xx:/tmp/
```

使用 `tools/deploy.sh` 辅助脚本可将上述两步合二为一：

```bash
./tools/deploy.sh build-aarch64/examples/hello_world/hello_world 192.168.x.xx
```

设备的默认凭证：IP 为 `192.168.x.xx`，用户名为 `root`，密码为 `recamera.1`。

---

## 在设备上运行 (Running on Device)

```bash
ssh root@192.168.x.xx
LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/hello_world
```

`LD_LIBRARY_PATH` 前缀能让动态链接器找到：
- 位于 `/tmp` 的 SDK 库 (`libreCamera_pro_sdk.so.1`)
- 位于 `/oem/usr/lib` 的 Rockchip 平台库（如 `librknnrt.so`、`librkaiq.so`、`librga.so`、`librockchip_mpp.so`）

### 管理 rkipc 进程

在运行多数摄像头演示程序之前，必须先停止系统的摄像头进程（`rkipc`），因为它占用了 `/dev/video13` 设备节点：

```bash
# 在运行摄像头演示程序前停止进程
/oem/usr/bin/RkLunch-stop.sh

# 测试完成后重启进程
/oem/usr/bin/RkLunch.sh
```

`isp_control` 演示程序是个例外——它需要 `rkipc` 处于**运行状态**，以便能够访问 rkaiq 上下文。

---

## 可用示例 (Available Examples)

| 示例 (Example) | 说明 (Description) | rkipc 状态 (rkipc state) |
|---------|-------------|------------|
| [hello_world](examples/hello_world/) | SDK 版本信息、设备信息、CPU 温度 | 运行或停止均可 |
| [gpio_control](examples/gpio_control/) | GPIO 输出（LED 闪烁）及输入轮询 | 运行或停止均可 |
| [uart_echo](examples/uart_echo/) | /dev/ttyS2 上的 UART 串口回显 | 运行或停止均可 |
| [imu_read](examples/imu_read/) | 50 Hz IMU 采样：加速度、陀螺仪、温度 | 运行或停止均可 |
| [camera_preview](examples/camera_preview/) | 30 fps 的 1920x1080 H.264 预览 | 需停止 |
| [capture_image](examples/capture_image/) | JPEG + NV12 单帧快照抓取 | 需停止 |
| [record_video](examples/record_video/) | 原生 H.264 Annex-B 文件录制 | 需停止 |
| [rknn_yolo](examples/rknn_yolo/) | 基于 NPU 的 YOLOX-S 目标检测 (~14 FPS) | 需停止 |
| [isp_control](examples/isp_control/) | 曝光、白平衡、色彩、降噪、HDR 参数调节 | 需运行 |
| [rtsp_server](examples/rtsp_server/) | 位于 8554 端口的 RTSP 流媒体服务器 | 需停止 |
| [rtmp_push](examples/rtmp_push/) | 向外部服务器推送 RTMP 数据流 | 需停止 |
| [onvif_server](examples/onvif_server/) | 位于 8899 端口的 ONVIF Profile S 发现服务器 | 需停止 |

---

## 添加新演示程序 (Adding New Demos)

### 第 1 步 — 创建演示程序目录

```bash
mkdir examples/my_demo
```

### 第 2 步 — 编写 main.cpp

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

### 第 3 步 — 添加 CMakeLists.txt

```cmake
# examples/my_demo/CMakeLists.txt
add_executable(my_demo main.cpp)
target_link_libraries(my_demo PRIVATE rv1126b_sdk::rv1126b_sdk)
```

### 第 4 步 — 在 examples/CMakeLists.txt 中注册

```cmake
add_subdirectory(my_demo)
```

### 第 5 步 — 引入新库（如需）

**情况 A — 库已存在于设备上（例如 libasound）：**

```cmake
# cmake/FindAlsa.cmake
find_library(ALSA_LIBRARY asound HINTS "${CMAKE_SYSROOT}/usr/lib")
find_path(ALSA_INCLUDE_DIR alsa/asoundlib.h HINTS "${CMAKE_SYSROOT}/usr/include")
```

将其添加到 `src/CMakeLists.txt`：

```cmake
target_link_libraries(reCamera_pro_sdk_shared PRIVATE ${ALSA_LIBRARY})
```

**情况 B — 新库尚未存在于设备上：**

1. 将 `.so` 复制到 `sysroot/oem/usr/lib/` 以供宿主机端链接使用
2. 将头文件复制到 `sysroot/oem/usr/include/yourlib/`
3. 将 `.so` 复制到设备：`scp yourlib.so root@192.168.x.xx:/tmp/`
4. 编写 `cmake/FindYourLib.cmake` 文件

**情况 C — 纯头文件库 (Header-only library)（例如 nlohmann/json）：**

```bash
cp -r nlohmann third_party/
```

```cmake
target_include_directories(reCamera_pro_sdk_shared PRIVATE ${CMAKE_SOURCE_DIR}/third_party)
```

### 第 6 步 — 编译并部署

```bash
./scripts/build.sh release --examples
./tools/deploy.sh build-aarch64/examples/my_demo/my_demo 192.168.x.xx
```

---

## API 概览 (API Overview)

所有的 API 调用都会返回 `Result<T>` 类型。在通过 `*result` 取值之前，请务必使用 `.ok()` 检查其状态。可使用 `rv1126b::to_string(result.error())` 读取易于理解的错误信息。

| 头文件 (Header) | 类 (Class) | 核心方法 (Key Methods) |
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

## 已知局限性 (Known Limitations)

- **ISP 控制依赖 rkipc 进程**：ISP 模块封装了 `librkaiq.so`，但完整的流水线归 `rkipc` 所有。在未提供处于活动状态下的、由 rkipc 提供的 rkaiq 上下文时调用 setter 方法会返回错误。
- **摄像头与 rkipc 的冲突**：Camera 模块会打开 `/dev/video13`（ISP 输出，多平面 NV12）。这与占用同一设备节点的 rkipc 存在冲突。因此在运行任何摄像头演示程序前必须停止 rkipc。
- **RTSP 8554 端口**：原厂固件上的 `go2rtc` 占用了 554 端口，因此 SDK 默认使用 8554 端口。
- **RKNN 模型路径**：YOLO 演示程序默认路径为 `/oem/usr/share/model/rknn/yolox_s.rknn`。如该文件缺失，请手动将模型复制到该位置。
- **ISP 初始化需要 IQ 文件**：`isp.init()` 会在 `/oem/usr/share/iqfiles` 目录下查找 IQ 调节文件。
- **GPIO 引脚编号**：可用引脚会因开发板型号不同而有所差异。在代码中硬编码引脚号前，请在设备上通过 `cat /sys/kernel/debug/gpio` 命令确认。
- **NV12 直接输出**：`capture_image` 演示程序试图通过帧回调获取 NV12 格式帧，然而 NV12 格式是否可用取决于运行时编码器流水线的配置情况。

---

## 许可证 (License)

Copyright (c) 2026 RV1126B SDK Project. All Rights Reserved.

**双重许可政策 (Dual License Policy):**

1. **非商业 / 教育用途 (Non-Commercial / Educational Use):**
   本 SDK 开放给非商业、个人、研究及教育目的使用。在上述条件范围内，您可以自由使用、修改及分发本代码。

2. **商业用途 (Commercial Use):**
   **严禁在未获得有效商业许可的情况下** 将本代码用于商业用途（包含但不限于将此 SDK 集成到商业产品中、将其用于创收服务，或为了获取利润而分发）。
   
关于商业许可的咨询，请联系该仓库的所有者或 reCamera Pro 团队。
