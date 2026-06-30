[English](README.md) | [中文](README_zh.md)

# rtsp_server

开启摄像头，分辨率设为 1920×1080、编码格式为 H.264、帧率为 30 fps，在端口 8554 上启动一个嵌入式 RTSP 服务器，并向任意 RTSP 客户端推送实时视频流。每隔 5 秒钟，它会打印当前活跃的客户端连接数以及推送的总帧数。按 Ctrl+C 停止运行。

---

## 前置条件

在运行此演示程序之前，**必须停止 rkipc 服务**。

```bash
/oem/usr/bin/RkLunch-stop.sh
```

在原厂固件中，端口 554 被 `go2rtc` 占用。因此，本演示程序默认使用 **8554** 端口。

测试完成后，重启 rkipc 服务：

```bash
/oem/usr/bin/RkLunch.sh
```

## 硬件要求

无需额外硬件。连接一台客户端设备（PC、手机或同一网络中的其他设备）即可观看 RTSP 视频流。

---

## 预期输出

```
=== RTSP streaming server demo ===

Camera opened (1920x1080 H.264 30fps)

Stream URL: rtsp://192.168.x.xx/live
Play with: ffplay rtsp://192.168.x.xx/live

RTSP server running, press Ctrl+C to stop...

[Status] Pushed: 150 frames  Active clients: 1
[Status] Pushed: 300 frames  Active clients: 1
...

RTSP server stopped.
```

测试数据：在有 1 个客户端连接的情况下，6 秒内推送了 146 帧。

---

## 如何编译

```bash
./scripts/build.sh release --examples
```

编译产物路径：`build-aarch64/examples/rtsp_server/rtsp_server`

---

## 如何部署和运行

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/rtsp_server/rtsp_server root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/rtsp_server"
```

从主机端观看视频流：

```bash
ffplay rtsp://192.168.x.xx:8554/live
# 或者使用 VLC：
vlc rtsp://192.168.x.xx:8554/live
```

---

## 关键代码解析

**RTSP 服务器配置** — 在调用 `start()` 之前设置端口、路径和编解码器等参数。这些参数必须与摄像头的配置保持一致：

```cpp
rv1126b::RTSPServer rtsp;
rv1126b::RTSPServer::Config rtsp_cfg;
rtsp_cfg.port   = 8554;
rtsp_cfg.path   = "/live";
rtsp_cfg.codec  = rv1126b::CodecType::H264;
rtsp_cfg.width  = 1920;
rtsp_cfg.height = 1080;
rtsp_cfg.fps    = 30;
rtsp.start(rtsp_cfg);
```

**推送帧数据** — 摄像头的每一帧都会通过一个单调递增的 PTS（显示时间戳）被推送到 RTSP 服务器。`is_keyframe` 标志位使客户端能够在 IDR 帧上进行同步：

```cpp
uint64_t pts_us = 0;
constexpr uint64_t PTS_STEP_US = 1000000 / 30;  // 在 30 fps 下，每帧约 33333 微秒

camera.set_frame_callback([&](const rv1126b::VideoFrame& frame) {
    rtsp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
    pts_us += PTS_STEP_US;
});
```

**设备 IP 检测** — 该演示程序通过 `getifaddrs()` 读取所有非本地环回的 IPv4 地址，以打印正确的视频流 URL。在 reCamera 设备上，默认的 IP 格式通常是 `192.168.x.xx`。

**客户端计数** — `rtsp.client_count()` 会返回当前活动的 RTSP 会话数，以便进行监控：

```cpp
int clients = rtsp.client_count();
printf("[Status] Pushed: %llu frames  Active clients: %d\n", frame_count, clients);
```

---

## 常见问题排查

| 现象 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `open camera failed` | rkipc 仍在运行 | 执行 `/oem/usr/bin/RkLunch-stop.sh` |
| `RTSP server start failed` | 8554 端口被占用 | 运行 `ss -tlnp \| grep 8554` 并停止冲突的进程 |
| `ffplay` 无法连接 | 防火墙限制或 IP 错误 | 使用 `ip addr` 确认设备 IP；用 `ping 192.168.x.xx` 测试网络连通性 |
| 视频流可以播放但卡顿 | 启动时未接收到关键帧 | 在连接客户端之前增加短暂的延迟（1-2 秒），编码器在启动时会生成一个 IDR 帧 |
| 延迟过高（>2 秒） | RTSP 客户端缓冲区过大 | 使用 `ffplay -fflags nobuffer -flags low_delay rtsp://...` 命令播放 |
