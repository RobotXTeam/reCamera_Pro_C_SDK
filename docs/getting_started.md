# Getting Started with the RV1126B SDK

This guide walks you through setting up a cross-compilation environment,
building the SDK, deploying it to a device, and running your first example.

---

## 1. Host System Requirements

- OS: Ubuntu 20.04 LTS or later (x86_64)
- CMake: >= 3.16
- Ninja or Make
- SSH client (`ssh`, `scp`)

Install build dependencies:

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake ninja-build \
    git wget curl \
    python3 python3-pip \
    libssl-dev zlib1g-dev
```

---

## 2. Toolchain Setup

The RV1126B uses an AArch64 Rockchip toolchain. The toolchain is bundled
with the RV1126B SDK root and lives under:

```
/path/to/your/workspace/
  linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/
```

Verify the compiler is present:

```bash
ls /path/to/your/workspace/\
linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/\
bin/aarch64-rockchip1240-linux-gnu-gcc
```

Add it to PATH for the current session:

```bash
export PATH="/path/to/your/workspace/\
linux-ipc-tools-toolchain-aarch64-rockchip1240-linux-gnu-main/bin:$PATH"
```

Verify:

```bash
aarch64-rockchip1240-linux-gnu-gcc --version
```

---

## 3. SDK Sysroot

The device libraries needed for linking are in the sysroot directory bundled
with the SDK:

```
/path/to/your/workspace/rv1126b_sdk/sysroot/
```

The CMake toolchain file (`cmake/toolchain-aarch64.cmake`) references this
path automatically — no manual configuration needed.

---

## 4. Building the SDK

### Using the build script (recommended)

```bash
cd /path/to/your/workspace/rv1126b_sdk
./scripts/build.sh release --examples
```

Output binaries land in `build-aarch64/examples/`.

### Manual CMake build

```bash
cd /path/to/your/workspace/rv1126b_sdk
mkdir -p build-aarch64
cd build-aarch64

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_EXAMPLES=ON \
  -GNinja

ninja -j$(nproc)
```

Build artifacts:

```
build-aarch64/
├── lib/
│   └── librv1126b_sdk.so
└── examples/
    ├── hello_world
    ├── gpio_control
    ├── uart_echo
    └── ...
```

---

## 5. Deploying to Device

The device is accessed over SSH. Default credentials:

| Field | Value |
|---|---|
| IP | 192.168.x.xx |
| User | root |
| Password | recamera.1 |

### Copy a binary

```bash
scp build-aarch64/examples/hello_world root@192.168.x.xx:/tmp/
```

### Use the deploy script

```bash
./tools/deploy.sh build-aarch64/examples/hello_world 192.168.x.xx
```

The script copies the binary, sets executable permission, and can optionally
run it over SSH.

---

## 6. Running on Device

```bash
ssh root@192.168.x.xx
/tmp/hello_world
```

Expected output:

```
========================================
  RV1126B SDK  v1.0.0
========================================

设备名称  : ReCam
芯片型号  : RV1126B
内核版本  : 4.19.232
NPU 核心数: 3
CPU 温度  : 42.3 °C

Hello, RV1126B SDK!
```

---

## 7. First Example Walkthrough

Open `examples/hello_world/main.cpp`. The program does four things:

1. Configures the logger to `Debug` level — all SDK log messages will be visible.
2. Calls `rv1126b::SysInfo::get_info()` and prints the returned `SysInfoData` struct.
3. Every call returns a `Result<T>`. Check `.ok()` before dereferencing with `*`.
4. Prints a greeting string.

The `Result<T>` pattern is used throughout the SDK. Always check the result
before using the value:

```cpp
auto res = rv1126b::SysInfo::get_info();
if (!res.ok()) {
    fprintf(stderr, "Error: %s\n", res.error().message().c_str());
    return EXIT_FAILURE;
}
auto& info = *res;        // safe to use after ok() check
```

---

## 8. Next Steps

- `examples/gpio_control` — GPIO output and input polling
- `examples/camera_preview` — camera setup and frame callbacks
- `examples/rknn_yolo` — NPU inference with YOLOX
- `docs/architecture.md` — design decisions and module dependency map
- `docs/cross_compilation.md` — detailed cross-compilation reference
