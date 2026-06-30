[English](README.md) | [中文](README_zh.md)

# rknn_yolo

使用摄像头画面在 NPU (RKNN SDK 2.3.2) 上运行 YOLOX-S 目标检测。对每一帧 NV12 图像进行预处理，将其转换为 640×640 的 RGB888 格式，在 NPU 上运行推理，应用 NMS（非极大值抑制）后处理，并打印检测到的目标及其类别名称、置信度得分和边界框。采用隔帧处理的方式以保持较低的 CPU 使用率。

---

## 前置条件

在运行此演示程序之前，**必须停止 rkipc**。

```bash
/oem/usr/bin/RkLunch-stop.sh
```

设备上必须存在 RKNN 模型文件：

```bash
ls /oem/usr/share/model/rknn/yolox_s.rknn
```

如果缺少该模型，请从 SDK 发布的 assets 中复制，或使用 RKNN Toolkit 将 ONNX 模型转换得到。

测试完成后重启 rkipc：

```bash
/oem/usr/bin/RkLunch.sh
```

## 硬件要求

无需额外硬件。RV1126B SoC 已集成 NPU。

---

## 预期输出

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

8 秒内大约完成 116 次推理（在半帧推理速率下约为 14 FPS）。确切的 FPS 取决于场景复杂度以及 NMS 后处理的负载。

---

## 如何构建

```bash
./scripts/build.sh release --examples
```

编译生成的二进制文件：`build-aarch64/examples/rknn_yolo/rknn_yolo`

---

## 如何部署和运行

```bash
ssh root@192.168.x.xx "/oem/usr/bin/RkLunch-stop.sh"
scp build-aarch64/examples/rknn_yolo/rknn_yolo root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/rknn_yolo"
```

---

## 关键代码解析

**NPU 初始化与模型加载** — NPU 通过 `dlopen` 封装了 `librknnrt.so`。`load_model()` 会返回一个模型句柄（model handle），必须将其传递给所有后续的推理调用：

```cpp
rv1126b::NPU npu;
npu.init();
auto load_res = npu.load_model("/oem/usr/share/model/rknn/yolox_s.rknn");
rv1126b::ModelHandle model_handle = *load_res;
```

**NV12 到 RGB888 的预处理** — 摄像头输出 NV12 帧。本演示程序使用最近邻缩放和 BT.601 全范围 YUV 到 RGB 转换，在 CPU 上将每一帧转换为 640×640 的 RGB888 格式：

```cpp
// Y 平面：src[0 .. src_w*src_h-1]
// UV 交错：src[src_w*src_h .. src_w*src_h*3/2-1]
int c = y_val - 16;
int d = u_val - 128;
int e = v_val - 128;
int r = (298*c + 409*e + 128) >> 8;
int g = (298*c - 100*d - 208*e + 128) >> 8;
int b = (298*c + 516*d + 128) >> 8;
```

**推理** — 输入/输出张量使用 `npu::TensorBuffer`。在消费完结果后调用 `release_outputs()` 以释放 NPU 内存：

```cpp
std::vector<rv1126b::npu::TensorBuffer> inputs = {input_tensor};
std::vector<rv1126b::npu::TensorBuffer> outputs;
npu.infer(model_handle, inputs, outputs);
// ... 解析 outputs[0] ...
npu.release_outputs(model_handle, outputs);
```

**YOLOX-S 输出格式** — 模型生成一个扁平的 `[num_anchors, 5 + num_classes]` 张量。每一行包含：`[cx, cy, w, h, obj_conf, class_0_prob, ..., class_79_prob]`。本演示程序通过一个简单的 NMS 过程对其进行解码。

**隔帧推理** — `frame_count % 2 != 0` 用于跳过奇数帧。这使得可用于后处理的 CPU 时间大致增加了一倍，同时将视觉延迟保持在 67 毫秒以下。

**清理工作** — 退出前务必调用 `unload_model()` 和 `deinit()`：

```cpp
camera.stop();
npu.unload_model(model_handle);
npu.deinit();
```

---

## 故障排除

| 症状 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `load model failed`（加载模型失败） | 找不到模型文件 | 验证 `ls /oem/usr/share/model/rknn/yolox_s.rknn` |
| `NPU init failed`（NPU 初始化失败） | 找不到 `librknnrt.so` | 确保 `LD_LIBRARY_PATH` 包含 `/oem/usr/lib` |
| `open camera failed`（打开摄像头失败） | rkipc 仍在运行 | 执行 `/oem/usr/bin/RkLunch-stop.sh` |
| FPS 低于 10 | 在 NV12 到 RGB 转换时遇到 CPU 瓶颈 | 基于 CPU 的转换仅为参考实现；在生产环境请使用 RGA 硬件加速 |
| 没有检测到目标 | 场景中不包含 COCO 类别目标，或置信度阈值过高 | 将 `CONF_THRESH` 降低至 0.25，并在视场内放置人或常见目标进行测试 |
| 边界框错位 | 缩放因子不正确 | 验证传递给 `parse_yolox_output` 的输入分辨率与实际的摄像头帧尺寸是否匹配 |
