[English](README.md) | [中文](README_zh.md)

# rtmp_push

Opens the camera at 1280×720 H.264 30 fps (2 Mbps) and pushes a live RTMP stream to an external server. The RTMP server URL is passed as a command-line argument. If the connection is lost, the demo automatically reconnects after 3 seconds. Prints per-second statistics including FPS, bitrate, total frames pushed, and reconnection count.

---

## Prerequisites

**rkipc must be stopped** before running this demo.

```bash
/oem/usr/bin/RkLunch-stop.sh
```

An RTMP server must be reachable from the device. Options:
- **nginx-rtmp**: run `docker run -p 1935:1935 tiangolo/nginx-rtmp` on the host
- **OBS Studio**: use the "Custom..." RTMP output in Stream settings
- **SRS** (Simple Realtime Server): `docker run -p 1935:1935 ossrs/srs:5`
- **YouTube / Twitch**: use the provided RTMP ingest URL and stream key

Restart rkipc after testing:

```bash
/oem/usr/bin/RkLunch.sh
```

## Hardware Required

No additional hardware. Ensure network connectivity from the device to the RTMP server.

---

## Expected Output

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

Tested: 146 frames pushed in ~6 seconds with no disconnections.

---

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/rtmp_push/rtmp_push`

---

## How to Deploy and Run

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/rtmp_push/rtmp_push root@192.168.x.xx:/tmp/

# Run with the target RTMP URL as argument
ssh root@192.168.x.xx \
  "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/rtmp_push rtmp://192.168.x.xx/live/stream"
```

View the stream on the host:

```bash
ffplay rtmp://192.168.x.xx/live/stream
# or with VLC:
vlc rtmp://192.168.x.xx/live/stream
```

---

## Key Code Walkthrough

**URL argument** — The RTMP server URL is provided on the command line:

```cpp
if (argc < 2) {
    fprintf(stderr, "Usage: %s <rtmp_url>\n", argv[0]);
    return EXIT_FAILURE;
}
const std::string rtmp_url = argv[1];
```

**Connecting** — `RTMPPusher::Config` mirrors the camera stream parameters. The codec, resolution, and frame rate must match what the camera produces:

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

**Pushing frames** — Each H.264 NAL unit is forwarded with a PTS. The PTS advances by `1000000 / fps` microseconds per frame:

```cpp
uint64_t pts_us = 0;
constexpr uint64_t PTS_STEP = 1000000 / 30;

camera.set_frame_callback([&](const rv1126b::VideoFrame& frame) {
    auto res = rtmp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
    pts_us += PTS_STEP;
    if (!res.ok()) {
        connected.store(false);  // signal reconnect thread
    }
});
```

**Auto-reconnect** — The main loop detects the `connected` flag going false and calls `disconnect()` + `connect()` again:

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

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `RTMP connect failed` | Server not reachable | `ping <server_ip>` from the device; verify port 1935 is open |
| `open camera failed` | rkipc still running | `/oem/usr/bin/RkLunch-stop.sh` |
| Stream connects but video is black | Camera did not start before first frame | Verify `camera.start()` returned `ok()` |
| Constant reconnects | Server is rejecting the stream key or path | Check server logs; verify the URL format (`rtmp://host/app/stream`) |
| High latency at viewer | Server buffer | Use a low-latency RTMP server config; for nginx-rtmp add `interleave on;` in the application block |
