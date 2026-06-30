[English](README.md) | [中文](README_zh.md)

# record_video

Records 10 seconds of 1920×1080 H.264 video at 30 fps (8 Mbps) to `/tmp/output.h264` as a raw Annex-B elementary stream. Prints per-second progress showing frame count, data volume, and running bitrate. Can be stopped early with Ctrl+C.

---

## Prerequisites

**rkipc must be stopped** before running this demo.

```bash
/oem/usr/bin/RkLunch-stop.sh
```

Restart after testing:

```bash
/oem/usr/bin/RkLunch.sh
```

## Hardware Required

No additional hardware. The camera module is integrated on the reCamera device.

---

## Expected Output

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

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/record_video/record_video`

---

## How to Deploy and Run

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/record_video/record_video root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/record_video"
```

Copy the output back to the host for playback:

```bash
scp root@192.168.x.xx:/tmp/output.h264 .
ffplay -f h264 output.h264
# Or remux to MP4 for wider compatibility:
ffmpeg -f h264 -i output.h264 -c copy output.mp4
```

---

## Key Code Walkthrough

**Frame writing in callback** — The frame callback writes each Annex-B NAL unit directly to the output file. No muxing or container is added:

```cpp
std::ofstream ofs("/tmp/output.h264", std::ios::binary | std::ios::trunc);

camera.set_frame_callback([&ofs, &stats](const rv1126b::VideoFrame& frame) {
    ofs.write(reinterpret_cast<const char*>(frame.data), frame.size);
    stats.total_frames++;
    stats.total_bytes += frame.size;
    if (frame.is_keyframe) stats.keyframes++;
});
```

**Annex-B format** — Each NAL unit begins with a start code (`00 00 00 01`). This is the format expected by `ffplay -f h264` and `ffmpeg -f h264`.

**10-second duration** — The main loop checks elapsed time; recording stops automatically after 10 seconds regardless of whether Ctrl+C is received:

```cpp
constexpr auto REC_DURATION = std::chrono::seconds(10);
while (!g_stop.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (std::chrono::steady_clock::now() - rec_start >= REC_DURATION) break;
    // ... print stats ...
}
camera.stop();
ofs.flush();
ofs.close();
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `open camera failed` | rkipc still running | `/oem/usr/bin/RkLunch-stop.sh` |
| Output file is empty | `camera.start()` failed | Check the error message; ensure rkipc is stopped |
| `ffplay` shows corrupted video | Partial write due to early exit | Always let the demo run to completion or Ctrl+C cleanly; the `ofs.flush()` call ensures all data is written |
| Recorded bitrate is much lower than 8 Mbps | Scene is uniform (e.g. lens cap on); H.264 CBR adapts to content | Normal for low-motion scenes |
