[English](README.md) | [中文](README_zh.md)

# hello_world

Minimal SDK smoke test: prints the SDK version, queries system information (device name, chip, kernel, NPU core count, CPU temperature), and exits. Use this to confirm the SDK library is correctly deployed and linked before running any other demo.

---

## Prerequisites

- SDK library deployed to `/tmp/` on the device (see root README).
- No other services need to be stopped or started.

## Hardware Required

No dedicated hardware beyond the reCamera device itself.

---

## Expected Output

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

## How to Build

From the SDK root:

```bash
./scripts/build.sh release --examples
```

The binary is produced at `build-aarch64/examples/hello_world/hello_world`.

---

## How to Deploy and Run

```bash
# Deploy (first time: also deploy the SDK library)
scp build-aarch64/src/libreCamera_pro_sdk.so.1.0.0 root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"
scp build-aarch64/examples/hello_world/hello_world root@192.168.x.xx:/tmp/

# Run
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/hello_world"
```

---

## Key Code Walkthrough

**Logger initialization** — The logger is a singleton. Setting the level to `Debug` makes all internal SDK messages visible during development:

```cpp
auto& log = rv1126b::Logger::get();
log.set_level(rv1126b::LogLevel::Debug);
```

**Result-type error handling** — Every SDK call returns `Result<T>`. The pattern used throughout all demos:

```cpp
auto result = rv1126b::sys::get_info();
if (!result.ok()) {
    fprintf(stderr, "Error: %s\n",
            std::string(rv1126b::to_string(result.error())).c_str());
    return EXIT_FAILURE;
}
const auto& info = *result;  // dereference only after .ok() check
```

**SDK version macro** — `RV1126B_SDK_VERSION_STRING` is defined in `include/rv1126b/version.hpp` at build time.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `error while loading shared libraries: libreCamera_pro_sdk.so.1` | SDK library not deployed or symlink missing | Run the deploy steps above; verify with `ls -la /tmp/libreCamera_pro_sdk.so*` on the device |
| `error while loading shared libraries: librknnrt.so` | Rockchip runtime missing from PATH | Ensure `LD_LIBRARY_PATH` includes `/oem/usr/lib` |
| CPU temp shows 0.0 °C | `/sys/class/thermal` path differs on this firmware | Expected; chip name field may also be blank on some firmware builds |
