[English](README.md) | [中文](README_zh.md)

# record_video

录制 10 秒的 1920×1080 30 fps (8 Mbps) H.264 视频，并将其作为原始的 Annex-B 基础流（elementary stream）保存到 `/tmp/output.h264`。程序会每秒打印一次进度，显示帧数、数据量和当前运行码率。可以通过按 Ctrl+C 提前停止。

---

## 前置条件

在运行此演示程序之前，**必须停止 rkipc**。

```bash
/oem/usr/bin/RkLunch-stop.sh
```

测试完成后重新启动：

```bash
/oem/usr/bin/RkLunch.sh
```

## 硬件要求

无需额外硬件。摄像头模组已集成在 reCamera 设备上。

---

## 预期输出

```
=== Video recording demo ===
Resolution: 1920x1080  Codec: H.264  FPS: 30  Bitrate: 8 Mbps
Output file: /tmp/output.h264

Recording for 10 seconds (Ctrl+C to stop early)...

[ 1.0s] Frames:    30  Keyframes:    1  Data: 0.98 MB  Bitrate: 8009 kbps
[ 2.0s] Frames:    60  Keyframes:    2  Data: 1.97 MB  Bitrate: 8012 kbps
...
[10.0s] Frames:   300  Keyframes:   10  Data: 9.84 MB  Bitrate: 7998 kbps

=== Recording complete ===
Duration   : 10.0 seconds
Total frames: 300
Total data  : 9.84 MB
Avg bitrate : 8003 kbps
Output file : /tmp/output.h264

Play: ffplay -f h264 /tmp/output.h264
Or remux: ffmpeg -f h264 -i /tmp/output.h264 -c copy /tmp/output.mp4
```

---

## 如何编译

```bash
./scripts/build.sh release --examples
```

二进制文件：`build-aarch64/examples/record_video/record_video`

---

## 如何部署与运行

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/record_video/record_video root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/record_video"
```

将输出文件复制回主机以进行播放：

```bash
scp root@192.168.x.xx:/tmp/output.h264 .
ffplay -f h264 output.h264
# 或者重新混流为 MP4 以获得更广泛的兼容性：
ffmpeg -f h264 -i output.h264 -c copy output.mp4
```

---

## 关键代码演练

**在回调函数中写入帧** — 帧回调函数将每个 Annex-B NAL 单元直接写入输出文件。未添加任何复用器或容器：

```cpp
std::ofstream ofs("/tmp/output.h264", std::ios::binary | std::ios::trunc);

camera.set_frame_callback([&ofs, &stats](const rv1126b::VideoFrame& frame) {
    ofs.write(reinterpret_cast<const char*>(frame.data), frame.size);
    stats.total_frames++;
    stats.total_bytes += frame.size;
    if (frame.is_keyframe) stats.keyframes++;
});
```

**Annex-B 格式** — 每个 NAL 单元都以起始码（`00 00 00 01`）开头。这是 `ffplay -f h264` 和 `ffmpeg -f h264` 所期望的格式。

**10 秒时长** — 主循环检查经过的时间；无论是否收到 Ctrl+C，录制都会在 10 秒后自动停止：

```cpp
constexpr auto REC_DURATION = std::chrono::seconds(10);
while (!g_stop.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (std::chrono::steady_clock::now() - rec_start >= REC_DURATION) break;
    // ... 打印统计数据 ...
}
camera.stop();
ofs.flush();
ofs.close();
```

---

## 故障排除

| 症状 | 可能的原因 | 解决方法 |
|---------|-------------|-----|
| `open camera failed` | rkipc 仍在运行 | 执行 `/oem/usr/bin/RkLunch-stop.sh` |
| 输出文件为空 | `camera.start()` 失败 | 检查错误信息；确保 rkipc 已停止 |
| `ffplay` 显示损坏的视频 | 提前退出导致写入不完整 | 始终让演示程序运行完成或干净地执行 Ctrl+C；`ofs.flush()` 调用可确保所有数据被写入 |
| 录制的码率远低于 8 Mbps | 场景内容单一（例如盖上了镜头盖）；H.264 CBR 会根据内容进行自适应调整 | 对于低运动场景来说属于正常现象 |
