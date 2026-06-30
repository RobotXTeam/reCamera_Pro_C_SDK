[English](README.md) | [中文](README_zh.md)

# isp_control

演示以编程方式控制 ISP 参数的功能：读取当前曝光，应用手动曝光（10 ms，ISO 400），将白平衡设置为 5500 K（日光），调整色彩参数（饱和度、锐度、对比度），开启强度为 70 的空域与时域降噪（spatial+temporal noise reduction），并开启 2× HDR 模式。3 秒后，它会恢复自动曝光、自动白平衡，并关闭 HDR。

---

## 前置要求

运行此演示**必须启动 rkipc**。ISP pipeline 和 rkaiq 上下文由 rkipc 管理。在未运行 rkipc 的情况下设置 ISP 参数将返回错误。

必须存在 IQ（图像质量）调优文件：

```bash
ls /oem/usr/share/iqfiles/
```

## 硬件需求

无需额外硬件。ISP 已集成在 RV1126B SoC 中。

---

## 预期输出

```
=== ISP control demo ===
IQ file dir: /oem/usr/share/iqfiles

ISP initialized

--- Current exposure params ---
  Exposure time : 8.231 ms
  ISO           : 200
  Analog gain   : 1.00
  Digital gain  : 1.00

--- Set manual exposure (10ms, ISO 400) ---
  Manual exposure set
--- Set white balance (5500K) ---
  White balance set to 5500K
--- Set color params (saturation 60, sharpness 55, contrast 58) ---
  Color params set
--- Set noise reduction (spatial+temporal, strength 70) ---
  NR params set
--- Enable 2x HDR mode ---
  HDR 2x enabled

Waiting 3 seconds to observe effect...
  3...
  2...
  1...

--- Restore automatic mode ---
  Auto exposure, auto white balance restored, HDR off

Program exit.
```

---

## 编译方法

```bash
./scripts/build.sh release --examples
```

二进制文件路径：`build-aarch64/examples/isp_control/isp_control`

---

## 部署与运行方法

```bash
# rkipc 必须处于运行状态（启动后的默认状态）
scp build-aarch64/examples/isp_control/isp_control root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/isp_control"
```

---

## 核心代码解读

**ISP 初始化** — `init()` 负责加载 `librkaiq.so` 并使用给定目录下的 IQ 文件连接到正在运行的 rkipc pipeline：

```cpp
rv1126b::ISP isp;
auto init_res = isp.init("/oem/usr/share/iqfiles");
```

**手动曝光** — `ExposureConfig` 用于选择模式并覆盖曝光时间和 ISO。`Manual` 模式会禁用自动曝光算法：

```cpp
rv1126b::ISP::ExposureConfig exp_cfg;
exp_cfg.mode    = rv1126b::ISP::ExposureMode::Manual;
exp_cfg.time_ms = 10.0f;
exp_cfg.iso     = 400;
isp.set_exposure(exp_cfg);
```

**白平衡** — `ColorTemp` 模式设置一个固定的色温（单位为开尔文）。典型值：2700 K（白炽灯），4000 K（荧光灯），5500 K（日光），6500 K（阴天）：

```cpp
rv1126b::ISP::WhiteBalanceConfig wb_cfg;
wb_cfg.mode       = rv1126b::ISP::WBMode::ColorTemp;
wb_cfg.color_temp = 5500;
isp.set_white_balance(wb_cfg);
```

**色彩参数** — 值域为 0–100，默认中性值为 50：

```cpp
rv1126b::ISP::ColorConfig color_cfg;
color_cfg.saturation = 60;  // 稍微更鲜艳
color_cfg.sharpness  = 55;
color_cfg.contrast   = 58;
isp.set_color(color_cfg);
```

**恢复自动模式** — 在退出前务必恢复自动模式，以避免相机在后续进程中处于预期外的状态：

```cpp
rv1126b::ISP::ExposureConfig auto_exp;
auto_exp.mode = rv1126b::ISP::ExposureMode::Auto;
isp.set_exposure(auto_exp);
rv1126b::ISP::WhiteBalanceConfig auto_wb;
auto_wb.mode = rv1126b::ISP::WBMode::Auto;
isp.set_white_balance(auto_wb);
isp.set_hdr_mode(rv1126b::ISP::HDRMode::Off);
```

---

## 故障排除

| 现象 | 可能原因 | 解决方法 |
|---------|-------------|-----|
| `ISP init failed` | rkipc 未运行 | 启动 rkipc：`/oem/usr/bin/RkLunch.sh`；等待 5 秒使其初始化 |
| `ISP init failed: IQ dir not found` | 缺少 IQ 文件 | 确认 `ls /oem/usr/share/iqfiles/*.json` 命令能返回文件 |
| 所有设置操作均返回警告 | rkipc 已启动，但 rkaiq 上下文尚未就绪 | 在启动 rkipc 后添加 2 秒延迟再运行此演示 |
| 2x HDR 失败但其他成功 | 传感器或固件在当前模式下不支持 HDR | 在某些配置上属于预期行为；HDR 需要传感器支持帧交织（frame interleaving） |
| 参数在视觉上似乎没有生效 | 更改应用于实时 rkipc 流，而不是此进程的输出 | 在运行演示时，通过观看 rkipc 的 RTSP 视频流以观察效果 |
