[English](README.md) | [中文](README_zh.md)

# camera_preview

通过 `/dev/video13`（ISP 输出，NV12 Multiplanar 格式）以 1920×1080 分辨率、H.264 编码、30 fps 打开相机，注册帧回调，并在 30 秒内每秒打印 FPS 统计信息。确认 ISP 图像处理管线能够以目标帧率提供稳定的视频流。

---

## 环境准备

运行此演示程序前**必须停止 rkipc 服务**。该服务会占用 `/dev/video13`，导致相机打开失败。

```bash
/oem/usr/bin/RkLunch-stop.sh
```

测试完成后，重新启动 rkipc：

```bash
/oem/usr/bin/RkLunch.sh
```

## 硬件要求

无需额外硬件。相机模块已集成在 reCamera 设备上。

---

## 预期输出

```
=== Camera preview demo ===
Resolution: 1920x1080  Codec: H.264  FPS: 30  Bitrate: 4 Mbps

Camera opened
Camera started, running 30 seconds (Ctrl+C to exit)...

[  1.0s] FPS:  30.0  Total frames:     30  Keyframes:     1
[  2.0s] FPS:  30.0  Total frames:     60  Keyframes:     2
[  3.0s] FPS:  30.0  Total frames:     90  Keyframes:     3
...
[ 30.0s] FPS:  30.0  Total frames:    900  Keyframes:    30

=== Frame stats ===
Total frames    : 900
Keyframes       : 30
Total data      : 14.32 MB
Avg frame size  : 16.35 KB

Program exit.
```

---

## 如何编译

```bash
./scripts/build.sh release --examples
```

可执行文件路径：`build-aarch64/examples/camera_preview/camera_preview`

---

## 如何部署与运行

```bash
# 首先停止 rkipc
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"

scp build-aarch64/examples/camera_preview/camera_preview root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/camera_preview"
```

---

## 核心代码解析

**相机配置** — `Camera::Config` 在调用 `open()` 之前收集所有的码流参数：

```cpp
rv1126b::Camera::Config cam_cfg;
cam_cfg.width        = 1920;
cam_cfg.height       = 1080;
cam_cfg.fps          = 30;
cam_cfg.codec        = rv1126b::CodecType::H264;
cam_cfg.bitrate_kbps = 4000;

rv1126b::Camera camera;
camera.open(cam_cfg);
```

**帧回调** — 回调函数在相机内部线程中运行。请确保该回调短小精悍且非阻塞；对于繁重的处理任务，请将其卸载至独立的工作线程进行处理：

```cpp
camera.set_frame_callback([&stats](const rv1126b::VideoFrame& frame) {
    std::lock_guard<std::mutex> lk(stats.mtx);
    stats.total_frames++;
    stats.frames_in_sec++;
    stats.total_bytes += frame.size;
    if (frame.is_keyframe) stats.keyframe_count++;
});
```

**启动与停止** — 在注册回调后调用 `start()`。调用 `stop()` 可刷新队列中未处理的帧并关闭设备：

```cpp
camera.start();
// ... 运行 30 秒 ...
camera.stop();
```

---

## 故障排除

| 症状 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `open camera failed` | rkipc 仍在运行 | 执行 `ps aux \| grep rkipc` 确认；运行 `/oem/usr/bin/RkLunch-stop.sh` 停止 |
| `open camera failed: device busy` | 其他进程占用了 `/dev/video13` | 使用 `fuser /dev/video13` 查找占用进程 |
| FPS 降至 28 以下 | 帧回调导致 CPU 过载 | 将处理逻辑移出回调，放入带有队列的独立工作线程中 |
| 输出中没有关键帧 | 编码器未启动 | 确保 `camera.start()` 返回 `ok()`；将日志级别设为 `LogLevel::Debug` 以查看日志 |
