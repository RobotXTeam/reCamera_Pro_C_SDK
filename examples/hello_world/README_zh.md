[English](README.md) | [中文](README_zh.md)

# hello_world

最简 SDK 冒烟测试：打印 SDK 版本，查询系统信息（设备名称、芯片、内核、NPU 核心数、CPU 温度）并退出。在运行任何其他演示（demo）之前，请使用此测试确认 SDK 库已正确部署并链接。

---

## 前置条件

- SDK 库已部署到设备上的 `/tmp/` 目录（请参阅根目录下的 README）。
- 不需要停止或启动其他任何服务。

## 硬件要求

除 reCamera 设备本身外，无需任何专用硬件。

---

## 预期输出

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

## 如何构建

从 SDK 根目录执行：

```bash
./scripts/build.sh release --examples
```

生成的二进制文件位于 `build-aarch64/examples/hello_world/hello_world`。

---

## 如何部署和运行

```bash
# 部署（首次：还需部署 SDK 库）
scp build-aarch64/src/libreCamera_pro_sdk.so.1.0.0 root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "ln -sf /tmp/libreCamera_pro_sdk.so.1.0.0 /tmp/libreCamera_pro_sdk.so.1"
scp build-aarch64/examples/hello_world/hello_world root@192.168.x.xx:/tmp/

# 运行
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/hello_world"
```

---

## 核心代码解读

**日志器初始化** — 日志器（Logger）是一个单例。将日志级别设置为 `Debug` 可在开发期间查看所有 SDK 内部消息：

```cpp
auto& log = rv1126b::Logger::get();
log.set_level(rv1126b::LogLevel::Debug);
```

**Result 类型的错误处理** — 每次 SDK 调用都会返回 `Result<T>`。所有演示代码中都采用以下模式：

```cpp
auto result = rv1126b::sys::get_info();
if (!result.ok()) {
    fprintf(stderr, "Error: %s\n",
            std::string(rv1126b::to_string(result.error())).c_str());
    return EXIT_FAILURE;
}
const auto& info = *result;  // 仅在确认 .ok() 后解引用
```

**SDK 版本宏** — `RV1126B_SDK_VERSION_STRING` 在编译期定义于 `include/rv1126b/version.hpp`。

---

## 故障排除

| 现象 | 可能原因 | 修复方法 |
|---------|-------------|-----|
| `error while loading shared libraries: libreCamera_pro_sdk.so.1` | 未部署 SDK 库或缺少软链接（symlink） | 运行上方的部署步骤；在设备上使用 `ls -la /tmp/libreCamera_pro_sdk.so*` 验证 |
| `error while loading shared libraries: librknnrt.so` | 环境变量 PATH 中缺少 Rockchip 运行时 | 确保 `LD_LIBRARY_PATH` 包含 `/oem/usr/lib` |
| CPU 温度显示为 0.0 °C | 在当前固件上 `/sys/class/thermal` 的路径不同 | 符合预期；在部分固件版本上，芯片名称字段也可能为空 |
