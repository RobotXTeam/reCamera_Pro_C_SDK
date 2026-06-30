[English](README.md) | [中文](README_zh.md)

# capture_image

以 1280×720 分辨率、15 fps 帧率开启摄像头（MJPEG 格式），并抓取一张 JPEG 快照保存到 `/tmp/snapshot.jpg`；随后启动视频帧回调函数，抓取一帧原始的 NV12 图像并保存到 `/tmp/snapshot.nv12`；最后再次抓取一张 JPEG 图像以验证摄像头是否正常工作。该示例适用于验证完整的图像抓取流水线，而无需录制长视频。

---

## 前置条件

在运行此演示之前，**必须停止 rkipc** 进程。

```bash
/oem/usr/bin/RkLunch-stop.sh
```

测试完成后重新启动：

```bash
/oem/usr/bin/RkLunch.sh
```

## 硬件要求

无需额外硬件。摄像头模块已集成在 reCamera 设备上。

---

## 预期输出

```
=== Image capture demo ===

Camera opened (1280x720)
Capturing first snapshot -> /tmp/snapshot.jpg ...
Snapshot 1 saved: /tmp/snapshot.jpg  (87.3 KB)

Capturing raw NV12 frame...
NV12 raw frame saved: /tmp/snapshot.nv12 (1382400 bytes)
  Convert with ffmpeg: ffmpeg -s 1280x720 -pix_fmt nv12 -i /tmp/snapshot.nv12 /tmp/snapshot_nv12.png

Capturing second snapshot -> /tmp/snapshot2.jpg ...
Snapshot 2 saved: /tmp/snapshot2.jpg  (86.9 KB)

Program exit.
```

---

## 如何编译

```bash
./scripts/build.sh release --examples
```

生成的可执行文件：`build-aarch64/examples/capture_image/capture_image`

---

## 如何部署与运行

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/capture_image/capture_image root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/capture_image"
```

将快照文件复制回主机进行检查：

```bash
scp root@192.168.x.xx:/tmp/snapshot.jpg .
scp root@192.168.x.xx:/tmp/snapshot.nv12 .
```

在主机上将 NV12 原始格式帧转换为 PNG 图片：

```bash
ffmpeg -f rawvideo -s 1280x720 -pix_fmt nv12 -i snapshot.nv12 snapshot_nv12.png
```

---

## 关键代码解析

**JPEG 快照** — `capture_jpeg()` 函数在内部打开设备，捕获一帧图像并编码为 JPEG 格式后写入文件。此调用不需要单独启动摄像头：

```cpp
rv1126b::Camera camera;
camera.open(cfg);
auto snap_res = camera.capture_jpeg("/tmp/snapshot.jpg");
```

**通过回调获取 NV12 原始数据帧** — 帧回调函数会接收到 `VideoFrame` 对象。NV12 格式帧会在 `frame.data` 中承载原始的 Y 和 UV 平面数据：

```cpp
camera.set_frame_callback([&](const rv1126b::VideoFrame& frame) {
    if (frame.pixel_format == rv1126b::PixelFormat::NV12) {
        std::ofstream ofs("/tmp/snapshot.nv12", std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(frame.data), frame.size);
        got_frame.store(true);
    }
});
camera.start();
// 最多等待 3 秒以获取一帧 NV12 数据
camera.stop();
```

**NV12 数据布局** — 对于 1280×720 分辨率的帧：Y 平面大小 = 1280×720 = 921600 字节，UV 交错平面大小 = 1280×360 = 460800 字节。总计 = 1382400 字节。

---

## 故障排除

| 现象 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `open camera failed` | rkipc 仍在运行 | 执行 `/oem/usr/bin/RkLunch-stop.sh` |
| JPEG 文件大小为 0 字节 | `capture_jpeg` 返回错误 | 启用 `LogLevel::Debug` 并检查错误字符串 |
| NV12 警告: did not receive NV12 frame | 摄像头在回调中输出的是已编码的 H.264，而非原始 NV12 | 在某些流水线配置下属于正常现象；请改用 JPEG 快照 |
| JPEG 文件显示异常 / 已损坏 | `/tmp` 目录存在写权限问题 | 验证 `/tmp` 是否可写：`touch /tmp/test && rm /tmp/test` |
