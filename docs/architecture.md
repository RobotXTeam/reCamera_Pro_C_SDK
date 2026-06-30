# SDK Architecture

## Layer Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     Application Layer                        │
│         (examples/, your application code)                   │
└────────────────────────────┬────────────────────────────────┘
                             │  C++17 API  (namespace rv1126b)
┌────────────────────────────▼────────────────────────────────┐
│                      SDK Public API                          │
│  GPIO  UART  I2C  PWM  ADC  IMU  Camera  ISP  NPU           │
│  VideoEncoder  RTSPServer  RTMPPusher  ONVIFServer           │
│  SysInfo  Watchdog  Logger                                   │
└──┬──────┬──────┬──────┬──────┬──────┬──────┬───────────────┘
   │      │      │      │      │      │      │
┌──▼──┐ ┌─▼──┐ ┌▼───┐ ┌▼───┐ ┌▼───┐ ┌▼──┐ ┌▼─────────────┐
│sysfs│ │term│ │V4L2│ │ISP1│ │MPP │ │IIO│ │  dlopen()     │
│gpio │ │ial │ │    │ │    │ │HW  │ │    │ │  librknn_api  │
└──┬──┘ └─┬──┘ └┬───┘ └┬───┘ └┬───┘ └┬──┘ └┬─────────────┘
   │      │     │      │      │      │     │
┌──▼──────▼─────▼──────▼──────▼──────▼─────▼──────────────┐
│                   Linux Kernel (4.19.x)                    │
│  gpio-rockchip  uart-8250  rkcif  rkisp1  rkvenc  iio     │
└──────────────────────────────────────────────────────────┘
┌──────────────────────────────────────────────────────────┐
│              RV1126B Hardware                             │
│  Cortex-A7 (quad)  ISP1  3-TOPS NPU  MPP VPU  GPIO...   │
└──────────────────────────────────────────────────────────┘
```

---

## Module Dependencies

```
ONVIFServer
  ├── RTSPServer
  └── Camera
          └── ISP

RTMPPusher
  └── (no internal dep — push raw frame data)

RTSPServer
  └── VideoEncoder (optional, when software encode path is used)

NPU
  └── dlopen(librknn_api.so)  ← loaded at runtime from /oem/usr/lib/

Camera
  ├── ISP (auto-initialized when iq_dir is provided in config)
  └── V4L2 (kernel rkcif driver)

VideoEncoder
  └── MPP (Rockchip Media Process Platform, /dev/mpp_service)

Logger
  └── (no dependencies — always available)

All modules
  └── Result<T>  (error propagation, zero overhead on success path)
```

---

## Design Decisions

### V4L2 for camera capture

The kernel `rkcif` driver exposes a standard V4L2 interface.
Using V4L2 means the SDK is portable to other V4L2-capable Rockchip
platforms (RV1109, RK3588) with minimal changes, and the kernel handles
DMA buffer management (DMABUF).

### dlopen for RKNN runtime

`librknn_api.so` ships with the firmware and its ABI changes between
firmware versions. Loading it at runtime with `dlopen` decouples the SDK
from a specific RKNN version — the correct library is resolved from
`/oem/usr/lib/` on the device at run time. A static link would require
recompiling the SDK for every firmware update.

### ISP integration

ISP tuning is highly sensor-specific. The `iq_dir` parameter passed to
`ISP::init()` points to per-sensor IQ XML files (`/oem/usr/share/iqfiles/`).
Separating IQ file loading from code allows sensor tuning to be updated
without recompiling the SDK.

### MPP for hardware video encoding

Rockchip MPP (Media Process Platform) provides a unified API over the
hardware VPU. Using MPP instead of a software encoder (libx264) delivers
H.264/H.265 encoding at negligible CPU cost, freeing cores for inference
and application logic.

---

## Error Handling Philosophy

Every fallible function returns `Result<T>`:

```cpp
template <typename T>
class Result {
public:
    bool   ok()    const;       // true on success
    T&     operator*();         // access value (UB if !ok())
    Error  error() const;       // error detail (message, code)
};
```

Design goals:

- **No exceptions** — exceptions have unpredictable overhead in embedded
  real-time code and interact poorly with C callbacks from V4L2/MPP.
- **Explicit error paths** — callers must check `.ok()` before using the
  value. This eliminates silent failures at the cost of a small amount of
  boilerplate.
- **Zero overhead on success** — on the success path `Result<T>` is
  equivalent to returning `T` directly; no heap allocation occurs.

Error codes are defined in `rv1126b::Error`:

| Code | Meaning |
|---|---|
| `Ok` | Success |
| `IoError` | I/O failure (file open, read, write) |
| `NotInitialized` | Module not initialized before use |
| `Timeout` | Operation exceeded time limit |
| `InvalidArgument` | Bad parameter value |
| `DeviceNotFound` | Hardware device not present |
| `NotSupported` | Feature not available on this hardware |
| `InternalError` | Unexpected SDK-internal error |

---

## Threading Model

Each module documents its own thread safety, but the general rules are:

- **Logger** is fully thread-safe. Multiple threads can log concurrently.
- **Camera** delivers frame callbacks on a dedicated internal capture thread.
  The callback must return quickly; heavy processing should be offloaded to
  a worker thread with a bounded queue.
- **NPU** inference (`infer()`) is not thread-safe. Serialize calls or use
  one `NPU` instance per thread.
- **RTSPServer / RTMPPusher** — `push_frame()` is thread-safe; it can be
  called from the camera callback thread.
- **GPIO / UART / I2C / PWM / ADC** — not thread-safe per instance.
  Use external synchronization if sharing an instance across threads.

Frame pipeline threading (typical deployment):

```
Capture thread (kernel DMA IRQ)
    │
    ▼
Camera callback (rv1126b internal thread)
    │
    ├─► Frame queue (bounded, lock-free SPSC)
    │         │
    │         ▼
    │   Inference thread   → NPU::infer()
    │
    └─► RTSPServer::push_frame()  (lock-free ring buffer internally)
```
