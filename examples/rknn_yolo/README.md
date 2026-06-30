[English](README.md) | [中文](README_zh.md)

# rknn_yolo

Runs YOLOX-S object detection on the NPU (RKNN SDK 2.3.2) using frames from the camera. Preprocesses each NV12 frame to RGB888 640×640, runs inference on the NPU, applies NMS post-processing, and prints detected objects with class name, confidence score, and bounding box. Processes every other frame to keep CPU usage low.

---

## Prerequisites

**rkipc must be stopped** before running this demo.

```bash
/oem/usr/bin/RkLunch-stop.sh
```

The RKNN model must be present on the device:

```bash
ls /oem/usr/share/model/rknn/yolox_s.rknn
```

If the model is missing, copy it from the SDK release assets or convert from ONNX using the RKNN Toolkit.

Restart rkipc after testing:

```bash
/oem/usr/bin/RkLunch.sh
```

## Hardware Required

No additional hardware. The RV1126B NPU is integrated in the SoC.

---

## Expected Output

```
=== RKNN YOLOX-S object detection demo ===
Model: /oem/usr/share/model/rknn/yolox_s.rknn
Input: 640x640  Conf threshold: 0.45  NMS IoU: 0.45

RKNN model loaded
Camera opened (1280x720)

Object detection running, press Ctrl+C to exit...

[Frame    2 | Infer  68 ms | FPS  7.1] Detected 2 objects
  person           conf=0.87  bbox=[142,38,498,712]
  chair            conf=0.72  bbox=[812,301,1178,680]
[Frame    4 | Infer  67 ms | FPS 14.2] Detected 1 objects
  person           conf=0.91  bbox=[140,35,502,715]
...

Total frames: 240  Total inferences: 120
Program exit.
```

Approximately 116 inferences in 8 seconds (~14 FPS at half-frame inference rate). Exact FPS depends on scene complexity and NMS post-processing load.

---

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/rknn_yolo/rknn_yolo`

---

## How to Deploy and Run

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/rknn_yolo/rknn_yolo root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/rknn_yolo"
```

---

## Key Code Walkthrough

**NPU initialization and model loading** — The NPU wraps `librknnrt.so` via `dlopen`. A model handle is returned by `load_model()` and must be passed to all subsequent inference calls:

```cpp
rv1126b::NPU npu;
npu.init();
auto load_res = npu.load_model("/oem/usr/share/model/rknn/yolox_s.rknn");
rv1126b::ModelHandle model_handle = *load_res;
```

**NV12 to RGB888 preprocessing** — The camera delivers NV12 frames. The demo converts each frame to RGB888 640×640 on the CPU using nearest-neighbor scaling and BT.601 full-range YUV-to-RGB:

```cpp
// Y plane: src[0 .. src_w*src_h-1]
// UV interleaved: src[src_w*src_h .. src_w*src_h*3/2-1]
int c = y_val - 16;
int d = u_val - 128;
int e = v_val - 128;
int r = (298*c + 409*e + 128) >> 8;
int g = (298*c - 100*d - 208*e + 128) >> 8;
int b = (298*c + 516*d + 128) >> 8;
```

**Inference** — Input/output tensors use `npu::TensorBuffer`. Call `release_outputs()` after consuming the results to free NPU memory:

```cpp
std::vector<rv1126b::npu::TensorBuffer> inputs = {input_tensor};
std::vector<rv1126b::npu::TensorBuffer> outputs;
npu.infer(model_handle, inputs, outputs);
// ... parse outputs[0] ...
npu.release_outputs(model_handle, outputs);
```

**YOLOX-S output format** — The model produces a flat `[num_anchors, 5 + num_classes]` tensor. Each row: `[cx, cy, w, h, obj_conf, class_0_prob, ..., class_79_prob]`. The demo decodes this with a simple NMS pass.

**Every-other-frame inference** — `frame_count % 2 != 0` skips odd-numbered frames. This roughly doubles available CPU time for post-processing while keeping visual latency under 67 ms.

**Cleanup** — Always call `unload_model()` and `deinit()` before exit:

```cpp
camera.stop();
npu.unload_model(model_handle);
npu.deinit();
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `load model failed` | Model file not found | Verify `ls /oem/usr/share/model/rknn/yolox_s.rknn` |
| `NPU init failed` | `librknnrt.so` not found | Ensure `LD_LIBRARY_PATH` includes `/oem/usr/lib` |
| `open camera failed` | rkipc still running | `/oem/usr/bin/RkLunch-stop.sh` |
| FPS is lower than 10 | CPU bottleneck in NV12→RGB conversion | The CPU-based conversion is a reference implementation; use RGA hardware acceleration for production use |
| No detections | Scene does not contain COCO objects, or confidence threshold too high | Lower `CONF_THRESH` to 0.25 and test with a person or common object visible |
| Bounding boxes are misaligned | Scale factor incorrect | Verify the input resolution passed to `parse_yolox_output` matches the actual camera frame dimensions |
