[English](README.md) | [中文](README_zh.md)

# camera_preview

Opens the camera at 1920×1080 H.264 30 fps via `/dev/video13` (ISP output, NV12 Multiplanar), registers a frame callback, and prints FPS statistics every second for 30 seconds. Confirms the ISP pipeline delivers a stable stream at the target frame rate.

---

## Prerequisites

**rkipc must be stopped** before running this demo. It holds `/dev/video13` and will cause the camera open to fail.

```bash
/oem/usr/bin/RkLunch-stop.sh
```

Restart rkipc after testing:

```bash
/oem/usr/bin/RkLunch.sh
```

## Hardware Required

No additional hardware. The camera module is integrated on the reCamera device.

---

## Expected Output

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

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/camera_preview/camera_preview`

---

## How to Deploy and Run

```bash
# Stop rkipc first
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"

scp build-aarch64/examples/camera_preview/camera_preview root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/camera_preview"
```

---

## Key Code Walkthrough

**Camera configuration** — `Camera::Config` gathers all stream parameters before `open()`:

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

**Frame callback** — The callback runs on the camera's internal thread. Keep it short and non-blocking; offload heavy processing to a worker thread:

```cpp
camera.set_frame_callback([&stats](const rv1126b::VideoFrame& frame) {
    std::lock_guard<std::mutex> lk(stats.mtx);
    stats.total_frames++;
    stats.frames_in_sec++;
    stats.total_bytes += frame.size;
    if (frame.is_keyframe) stats.keyframe_count++;
});
```

**Start / stop** — Call `start()` after registering the callback. Call `stop()` to flush pending frames and close the device:

```cpp
camera.start();
// ... run for 30 seconds ...
camera.stop();
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `open camera failed` | rkipc is still running | `ps aux \| grep rkipc`; run `/oem/usr/bin/RkLunch-stop.sh` |
| `open camera failed: device busy` | Another process holds `/dev/video13` | `fuser /dev/video13` to identify it |
| FPS drops below 28 | CPU overloaded by the frame callback | Move processing out of the callback into a separate thread with a queue |
| No keyframes in output | Encoder not started | Ensure `camera.start()` returns `ok()`; check logs with `LogLevel::Debug` |
