[English](README.md) | [中文](README_zh.md)

# capture_image

Opens the camera at 1280×720 MJPEG 15 fps, captures a JPEG snapshot to `/tmp/snapshot.jpg`, then starts a frame callback to capture a raw NV12 frame to `/tmp/snapshot.nv12`, and finally captures a second JPEG to verify the camera remains functional. Useful for verifying the full capture pipeline without recording a long video.

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

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/capture_image/capture_image`

---

## How to Deploy and Run

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/capture_image/capture_image root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/capture_image"
```

Copy the snapshots back to the host for inspection:

```bash
scp root@192.168.x.xx:/tmp/snapshot.jpg .
scp root@192.168.x.xx:/tmp/snapshot.nv12 .
```

Convert the NV12 raw frame to PNG on the host:

```bash
ffmpeg -f rawvideo -s 1280x720 -pix_fmt nv12 -i snapshot.nv12 snapshot_nv12.png
```

---

## Key Code Walkthrough

**JPEG snapshot** — `capture_jpeg()` opens the device internally, captures one frame, encodes it as JPEG, and writes the file. The camera does not need to be started separately for this call:

```cpp
rv1126b::Camera camera;
camera.open(cfg);
auto snap_res = camera.capture_jpeg("/tmp/snapshot.jpg");
```

**NV12 raw frame via callback** — The frame callback receives `VideoFrame` objects. NV12 frames carry the raw Y+UV planes in `frame.data`:

```cpp
camera.set_frame_callback([&](const rv1126b::VideoFrame& frame) {
    if (frame.pixel_format == rv1126b::PixelFormat::NV12) {
        std::ofstream ofs("/tmp/snapshot.nv12", std::ios::binary);
        ofs.write(reinterpret_cast<const char*>(frame.data), frame.size);
        got_frame.store(true);
    }
});
camera.start();
// wait up to 3 seconds for one NV12 frame
camera.stop();
```

**NV12 layout** — For a 1280×720 frame: Y plane = 1280×720 = 921600 bytes, UV interleaved plane = 1280×360 = 460800 bytes. Total = 1382400 bytes.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `open camera failed` | rkipc still running | `/oem/usr/bin/RkLunch-stop.sh` |
| JPEG file is 0 bytes | `capture_jpeg` returned an error | Enable `LogLevel::Debug` and check the error string |
| NV12 warning: did not receive NV12 frame | Camera outputs encoded H.264 in the callback, not raw NV12 | Expected in some pipeline configurations; use the JPEG snapshot instead |
| JPEG file looks wrong / corrupted | Write permissions issue on `/tmp` | Verify `/tmp` is writable: `touch /tmp/test && rm /tmp/test` |
