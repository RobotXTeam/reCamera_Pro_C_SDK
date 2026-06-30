[English](README.md) | [中文](README_zh.md)

# onvif_server

在 8899 端口启动一个符合 ONVIF Profile S 标准的设备服务，并结合在 8554 端口上的 RTSP 服务器，以 25 fps 的帧率流式传输 1920×1080 的 H.264 视频。ONVIF 服务会响应 WS-Discovery 探测，允许 IP 摄像机管理软件（如 ONVIF Device Manager、iSpy、Milestone 等）自动发现并将 reCamera 添加为网络摄像机。

---

## 准备工作

在运行此演示程序之前，**必须停止 rkipc**（因为摄像头模块使用 `/dev/video13`）。

```bash
/oem/usr/bin/RkLunch-stop.sh
```

测试完成后重新启动 rkipc：

```bash
/oem/usr/bin/RkLunch.sh
```

## 硬件要求

无需额外硬件。为了使 ONVIF 发现功能正常工作，请将 reCamera 和客户端设备连接到同一个网络中。

---

## 预期输出

```
=== ONVIF Profile S server demo ===

Camera opened (1920x1080 H.264 25fps)
RTSP server started (port 8554, path /live)

ONVIF device discovery URL : http://192.168.x.xx:8899/onvif/device_service
Media stream URL (RTSP)    : rtsp://192.168.x.xx/live

Test tools:
  ONVIF Device Manager (Windows)
  ffplay rtsp://192.168.x.xx/live

Press Ctrl+C to stop...

Stopping services...
Program exit.
```

---

## 如何编译

```bash
./scripts/build.sh release --examples
```

二进制文件路径：`build-aarch64/examples/onvif_server/onvif_server`

---

## 如何部署和运行

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/onvif_server/onvif_server root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/onvif_server"
```

从同一网络中的 Windows PC 测试 ONVIF 发现功能：

1. 下载 **ONVIF Device Manager**（免费，可从 SourceForge 获取）。
2. 启动该软件 — reCamera 应自动出现在设备列表中。
3. 双击设备以打开实时视频流。

通过 ffplay 测试：

```bash
ffplay rtsp://192.168.x.xx:8554/live
```

直接查询 ONVIF 设备服务：

```bash
curl -s http://192.168.x.xx:8899/onvif/device_service
```

---

## 核心代码演练

**架构** — 该演示程序组合了三个 SDK 对象：`Camera` → `RTSPServer` → `ONVIFServer`。ONVIF 服务将 RTSP URL 作为其媒体配置文件的流媒体 URI 进行广播。

**ONVIF 服务器配置** — 在调用 `start()` 之前配置设备元数据（名称、制造商、型号、固件版本）以及 RTSP 流媒体 URI：

```cpp
rv1126b::ONVIFServer onvif;
rv1126b::ONVIFServer::Config onvif_cfg;
onvif_cfg.port             = 8899;
onvif_cfg.name             = "RV1126B-CAM";
onvif_cfg.manufacturer     = "RV1126B SDK";
onvif_cfg.model            = "RV1126B";
onvif_cfg.firmware_version = RV1126B_SDK_VERSION_STRING;
onvif_cfg.serial_number    = "RV1126B-001";
onvif_cfg.rtsp_url         = "rtsp://" + device_ip + "/live";
onvif_cfg.width            = 1920;
onvif_cfg.height           = 1080;
onvif_cfg.fps              = 25;
onvif.start(onvif_cfg);
```

**摄像头 → RTSP 帧流水线** — 摄像头帧回调函数将每一帧推送到 RTSP 服务器，并带有一个单调递增的 PTS（显示时间戳）：

```cpp
uint64_t pts_us = 0;
constexpr uint64_t PTS_STEP = 1000000 / 25;

camera.set_frame_callback([&rtsp, &pts_us](const rv1126b::VideoFrame& frame) {
    rtsp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
    pts_us += PTS_STEP;
});
camera.start();
```

**优雅停机顺序** — 按依赖关系逆序停止：首先是 ONVIF，然后是 RTSP，最后是摄像头：

```cpp
onvif.stop();
rtsp.stop();
camera.stop();
```

---

## 故障排除

| 症状 | 可能的原因 | 解决方法 |
|---------|-------------|-----|
| `open camera failed` | rkipc 仍在运行 | 执行 `/oem/usr/bin/RkLunch-stop.sh` |
| `ONVIF server start failed: port in use` | 8899 端口已被占用 | 执行 `ss -tlnp \| grep 8899`；停止冲突的进程 |
| ONVIF Device Manager 无法找到设备 | WS-Discovery 多播被阻断 | 确保两台设备位于同一 L2 子网（它们之间没有 VLAN/防火墙）；尝试在 ONVIF Device Manager 中通过 IP 手动添加设备 |
| 视频流在 ONVIF 工具中显示，但卡顿严重 | RTSP 端口 8554 未开放 | 确认 `ss -tlnp \| grep 8554` 显示 RTSP 服务器正在监听 |
| ONVIF 工具显示 "unauthorized"（未授权） | 某些 ONVIF 客户端需要凭据 | 本演示未实现 WS-Security；请使用支持未认证访问的客户端，或者在 ONVIF 处理程序中添加摘要认证 |
