[English](README.md) | [中文](README_zh.md)

# rtsp_server

Opens the camera at 1920×1080 H.264 30 fps, starts an embedded RTSP server on port 8554, and streams live video to any RTSP client. Every 5 seconds it prints the number of active client connections and total frames pushed. Press Ctrl+C to stop.

---

## Prerequisites

**rkipc must be stopped** before running this demo.

```bash
/oem/usr/bin/RkLunch-stop.sh
```

Port 554 is occupied by `go2rtc` on the stock firmware. This demo uses port **8554** by default.

Restart rkipc after testing:

```bash
/oem/usr/bin/RkLunch.sh
```

## Hardware Required

No additional hardware. Connect a client device (PC, phone, or another device on the same network) to view the RTSP stream.

---

## Expected Output

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

Tested: 146 frames pushed in 6 seconds with one connected client.

---

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/rtsp_server/rtsp_server`

---

## How to Deploy and Run

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/rtsp_server/rtsp_server root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/rtsp_server"
```

View the stream from the host:

```bash
ffplay rtsp://192.168.x.xx:8554/live
# or with VLC:
vlc rtsp://192.168.x.xx:8554/live
```

---

## Key Code Walkthrough

**RTSP server configuration** — Set port, path, and codec parameters before calling `start()`. These must match the camera configuration:

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

**Pushing frames** — Each camera frame is forwarded to the RTSP server with a monotonically increasing PTS. The `is_keyframe` flag allows clients to sync on IDR frames:

```cpp
uint64_t pts_us = 0;
constexpr uint64_t PTS_STEP_US = 1000000 / 30;  // ~33333 µs per frame at 30 fps

camera.set_frame_callback([&](const rv1126b::VideoFrame& frame) {
    rtsp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
    pts_us += PTS_STEP_US;
});
```

**Device IP detection** — The demo reads all non-loopback IPv4 addresses via `getifaddrs()` to print the correct stream URL. On the reCamera the default IP is `192.168.x.xx`.

**Client count** — `rtsp.client_count()` returns the current number of active RTSP sessions for monitoring:

```cpp
int clients = rtsp.client_count();
printf("[Status] Pushed: %llu frames  Active clients: %d\n", frame_count, clients);
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `open camera failed` | rkipc still running | `/oem/usr/bin/RkLunch-stop.sh` |
| `RTSP server start failed` | Port 8554 already in use | `ss -tlnp \| grep 8554`; stop the conflicting process |
| `ffplay` cannot connect | Firewall or wrong IP | Verify device IP with `ip addr`; test connectivity with `ping 192.168.x.xx` |
| Stream plays but freezes | No keyframe received at start | Add a short delay (1–2 seconds) before connecting the client; the encoder produces an IDR at start-up |
| High latency (>2 seconds) | RTSP client buffer too large | Use `ffplay -fflags nobuffer -flags low_delay rtsp://...` |
