[English](README.md) | [中文](README_zh.md)

# rtmp_push

在 1280×720 H.264 30 fps (2 Mbps) 下打开摄像头，并将实时 RTMP 流推送到外部服务器。RTMP 服务器 URL 作为命令行参数传递。如果连接断开，演示程序会在 3 秒后自动重连。打印每秒的统计信息，包括 FPS、比特率、已推送的总帧数和重连次数。

---

## 前置条件

在运行此演示程序之前，**必须停止 rkipc**。

```bash
/oem/usr/bin/RkLunch-stop.sh
```

设备必须能够访问一个 RTMP 服务器。选项：
- **nginx-rtmp**：在主机上运行 `docker run -p 1935:1935 tiangolo/nginx-rtmp`
- **OBS Studio**：在“推流（Stream）”设置中使用“自定义（Custom...）” RTMP 输出
- **SRS** (Simple Realtime Server)：`docker run -p 1935:1935 ossrs/srs:5`
- **YouTube / Twitch**：使用提供的 RTMP 摄取 URL 和流密钥（stream key）

测试完成后重启 rkipc：

```bash
/oem/usr/bin/RkLunch.sh
```

## 硬件要求

无需额外硬件。确保设备与 RTMP 服务器之间的网络连接畅通。

---

## 预期输出

```
=== RTMP push demo ===
Push URL: rtmp://192.168.x.xx/live/stream

Camera opened (1280x720 H.264 30fps 2Mbps)
[RTMP] Connected: rtmp://192.168.x.xx/live/stream
Pushing, press Ctrl+C to stop...

Time(s)   FPS       Bitrate(kbps)  Total frames  Reconnects
---------------------------------------------------------
1.0       30.0      2003           30            0
2.0       30.0      1998           60            0
3.0       30.0      2005           90            0
...

=== Push summary ===
Total frames pushed : 146
Total data          : 3.54 MB
Reconnects          : 0
Program exit.
```

已测试：在约 6 秒内推送了 146 帧，未发生断开连接。

---

## 如何编译

```bash
./scripts/build.sh release --examples
```

二进制文件：`build-aarch64/examples/rtmp_push/rtmp_push`

---

## 如何部署和运行

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/rtmp_push/rtmp_push root@192.168.x.xx:/tmp/

# 将目标 RTMP URL 作为参数运行
ssh root@192.168.x.xx \
  "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/rtmp_push rtmp://192.168.x.xx/live/stream"
```

在主机上查看视频流：

```bash
ffplay rtmp://192.168.x.xx/live/stream
# 或使用 VLC：
vlc rtmp://192.168.x.xx/live/stream
```

---

## 关键代码解析

**URL 参数** — RTMP 服务器 URL 由命令行提供：

```cpp
if (argc < 2) {
    fprintf(stderr, "Usage: %s <rtmp_url>\n", argv[0]);
    return EXIT_FAILURE;
}
const std::string rtmp_url = argv[1];
```

**连接** — `RTMPPusher::Config` 镜像摄像头的视频流参数。编解码器、分辨率和帧率必须与摄像头生成的相匹配：

```cpp
rv1126b::RTMPPusher rtmp;
rv1126b::RTMPPusher::Config rtmp_cfg;
rtmp_cfg.url          = rtmp_url;
rtmp_cfg.width        = 1280;
rtmp_cfg.height       = 720;
rtmp_cfg.fps          = 30;
rtmp_cfg.bitrate_kbps = 2000;
rtmp_cfg.codec        = rv1126b::CodecType::H264;
rtmp.connect(rtmp_cfg);
```

**推送数据帧** — 每个 H.264 NAL 单元在转发时都会附带一个 PTS。PTS 每帧推进 `1000000 / fps` 微秒：

```cpp
uint64_t pts_us = 0;
constexpr uint64_t PTS_STEP = 1000000 / 30;

camera.set_frame_callback([&](const rv1126b::VideoFrame& frame) {
    auto res = rtmp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
    pts_us += PTS_STEP;
    if (!res.ok()) {
        connected.store(false);  // 触发重连线程
    }
});
```

**自动重连** — 主循环检测到 `connected` 标志变为 false，并调用 `disconnect()` + 再次 `connect()`：

```cpp
if (!connected.load() && !g_stop.load()) {
    rtmp.disconnect();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    if (connect_rtmp()) {
        connected.store(true);
        stats.reconnects++;
    }
}
```

---

## 故障排除

| 症状 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `RTMP connect failed` | 服务器不可达 | 从设备上执行 `ping <server_ip>`；验证 1935 端口是否开放 |
| `open camera failed` | rkipc 仍在运行 | 执行 `/oem/usr/bin/RkLunch-stop.sh` |
| 视频流连接成功但黑屏 | 摄像头在第一帧之前未能启动 | 验证 `camera.start()` 是否返回了 `ok()` |
| 持续重连 | 服务器拒绝了流密钥或路径 | 检查服务器日志；验证 URL 格式（`rtmp://host/app/stream`） |
| 观众端延迟高 | 服务器缓冲区问题 | 使用低延迟 RTMP 服务器配置；对于 nginx-rtmp，在 application 块中添加 `interleave on;` |
