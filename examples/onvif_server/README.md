[English](README.md) | [中文](README_zh.md)

# onvif_server

Starts an ONVIF Profile S compliant device service on port 8899, combined with an RTSP server on port 8554, and streams 1920×1080 H.264 video at 25 fps. The ONVIF service responds to WS-Discovery probes, allowing IP camera management software (ONVIF Device Manager, iSpy, Milestone, etc.) to automatically discover and add the reCamera as a network camera.

---

## Prerequisites

**rkipc must be stopped** before running this demo (the camera module uses `/dev/video13`).

```bash
/oem/usr/bin/RkLunch-stop.sh
```

Restart rkipc after testing:

```bash
/oem/usr/bin/RkLunch.sh
```

## Hardware Required

No additional hardware. Connect the reCamera and a client device to the same network for ONVIF discovery to work.

---

## Expected Output

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

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/onvif_server/onvif_server`

---

## How to Deploy and Run

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/onvif_server/onvif_server root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/onvif_server"
```

Test ONVIF discovery from a Windows PC on the same network:

1. Download **ONVIF Device Manager** (free, from SourceForge).
2. Launch it — the reCamera should appear in the device list automatically.
3. Double-click the device to open the live stream.

Test via ffplay:

```bash
ffplay rtsp://192.168.x.xx:8554/live
```

Query the ONVIF device service directly:

```bash
curl -s http://192.168.x.xx:8899/onvif/device_service
```

---

## Key Code Walkthrough

**Architecture** — The demo composes three SDK objects: `Camera` → `RTSPServer` → `ONVIFServer`. The ONVIF service advertises the RTSP URL as its media profile stream URI.

**ONVIF server configuration** — Device metadata (name, manufacturer, model, firmware version) and the RTSP stream URI are configured before `start()`:

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

**Camera → RTSP frame pipeline** — The camera frame callback pushes each frame into the RTSP server with a monotonically increasing PTS:

```cpp
uint64_t pts_us = 0;
constexpr uint64_t PTS_STEP = 1000000 / 25;

camera.set_frame_callback([&rtsp, &pts_us](const rv1126b::VideoFrame& frame) {
    rtsp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
    pts_us += PTS_STEP;
});
camera.start();
```

**Graceful shutdown order** — Stop in reverse dependency order: ONVIF first, then RTSP, then camera:

```cpp
onvif.stop();
rtsp.stop();
camera.stop();
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `open camera failed` | rkipc still running | `/oem/usr/bin/RkLunch-stop.sh` |
| `ONVIF server start failed: port in use` | Port 8899 already bound | `ss -tlnp \| grep 8899`; stop the conflicting process |
| ONVIF Device Manager cannot find the device | WS-Discovery multicast blocked | Ensure both devices are on the same L2 subnet (no VLAN/firewall between them); try adding the device manually by IP in ONVIF Device Manager |
| Stream shows in ONVIF tool but is choppy | RTSP port 8554 not open | Verify `ss -tlnp \| grep 8554` shows the RTSP server listening |
| ONVIF tool shows "unauthorized" | Some ONVIF clients require credentials | The demo does not implement WS-Security; use a client that supports unauthenticated access, or add digest auth in the ONVIF handler |
