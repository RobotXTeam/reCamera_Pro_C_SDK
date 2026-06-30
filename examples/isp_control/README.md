[English](README.md) | [中文](README_zh.md)

# isp_control

Demonstrates programmatic control of ISP parameters: reads the current exposure, applies manual exposure (10 ms, ISO 400), sets white balance to 5500 K (daylight), adjusts color parameters (saturation, sharpness, contrast), enables spatial+temporal noise reduction at strength 70, and enables 2× HDR mode. After 3 seconds it restores automatic exposure, auto white balance, and disables HDR.

---

## Prerequisites

**rkipc must be running** for this demo. The ISP pipeline and rkaiq context are managed by rkipc. Setting ISP parameters without rkipc returns an error.

IQ (Image Quality) tuning files must be present:

```bash
ls /oem/usr/share/iqfiles/
```

## Hardware Required

No additional hardware. The ISP is integrated in the RV1126B SoC.

---

## Expected Output

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

## How to Build

```bash
./scripts/build.sh release --examples
```

Binary: `build-aarch64/examples/isp_control/isp_control`

---

## How to Deploy and Run

```bash
# rkipc must be running (default state after boot)
scp build-aarch64/examples/isp_control/isp_control root@192.168.x.xx:/tmp/
ssh root@192.168.x.xx "LD_LIBRARY_PATH=/tmp:/oem/usr/lib:/usr/lib /tmp/isp_control"
```

---

## Key Code Walkthrough

**ISP initialization** — `init()` loads `librkaiq.so` and connects to the running rkipc pipeline using the IQ files in the given directory:

```cpp
rv1126b::ISP isp;
auto init_res = isp.init("/oem/usr/share/iqfiles");
```

**Manual exposure** — `ExposureConfig` selects the mode and overrides time and ISO. The `Manual` mode disables the auto-exposure algorithm:

```cpp
rv1126b::ISP::ExposureConfig exp_cfg;
exp_cfg.mode    = rv1126b::ISP::ExposureMode::Manual;
exp_cfg.time_ms = 10.0f;
exp_cfg.iso     = 400;
isp.set_exposure(exp_cfg);
```

**White balance** — `ColorTemp` mode sets a fixed color temperature in Kelvin. Typical values: 2700 K (tungsten), 4000 K (fluorescent), 5500 K (daylight), 6500 K (overcast):

```cpp
rv1126b::ISP::WhiteBalanceConfig wb_cfg;
wb_cfg.mode       = rv1126b::ISP::WBMode::ColorTemp;
wb_cfg.color_temp = 5500;
isp.set_white_balance(wb_cfg);
```

**Color parameters** — Values in the range 0–100 with 50 as the neutral default:

```cpp
rv1126b::ISP::ColorConfig color_cfg;
color_cfg.saturation = 60;  // slightly more vivid
color_cfg.sharpness  = 55;
color_cfg.contrast   = 58;
isp.set_color(color_cfg);
```

**Restore auto mode** — Always restore auto modes before exit to avoid leaving the camera in an unexpected state for subsequent processes:

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

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| `ISP init failed` | rkipc is not running | Start rkipc: `/oem/usr/bin/RkLunch.sh`; wait 5 seconds for it to initialize |
| `ISP init failed: IQ dir not found` | IQ files missing | Verify `ls /oem/usr/share/iqfiles/*.json` returns files |
| All setters return warnings | rkipc started but rkaiq context not yet ready | Add a 2-second delay after starting rkipc before running this demo |
| HDR 2x fails but others succeed | Sensor or firmware does not support HDR in the current mode | Expected on some configurations; HDR requires sensor frame interleaving support |
| Parameters do not seem to take effect visually | Changes apply to the live rkipc stream, not to this process's output | View the RTSP stream from rkipc while the demo is running to observe the effect |
