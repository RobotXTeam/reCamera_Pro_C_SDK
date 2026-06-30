/**
 * @file rv1126b.hpp
 * @brief RV1126B SDK — convenience all-in-one include
 *
 * Includes all SDK modules in one shot.
 * When the build system requires modularity, include individual sub-module headers instead.
 *
 * @code
 * #include <rv1126b/rv1126b.hpp>
 * using namespace rv1126b;
 * @endcode
 */
#pragma once

// ─── Meta ────────────────────────────────────────────────────────────────────────
#include "rv1126b/core/error.hpp"
#include "rv1126b/core/logger.hpp"
#include "rv1126b/core/config.hpp"
#include "rv1126b/core/utils.hpp"

// ─── Hardware modules ────────────────────────────────────────────────────────────
#include "rv1126b/camera/camera.hpp"
#include "rv1126b/isp/isp.hpp"
#include "rv1126b/video/encoder.hpp"
#include "rv1126b/video/recorder.hpp"
#include "rv1126b/npu/npu.hpp"
#include "rv1126b/npu/tensor.hpp"
#include "rv1126b/audio/audio.hpp"

// ─── Streaming ───────────────────────────────────────────────────────────────────
#include "rv1126b/stream/rtsp.hpp"
#include "rv1126b/stream/rtmp.hpp"
#include "rv1126b/stream/onvif.hpp"

// ─── Peripherals ─────────────────────────────────────────────────────────────────
#include "rv1126b/peripheral/gpio.hpp"
#include "rv1126b/peripheral/uart.hpp"
#include "rv1126b/peripheral/i2c.hpp"
#include "rv1126b/peripheral/spi.hpp"
#include "rv1126b/peripheral/pwm.hpp"
#include "rv1126b/peripheral/adc.hpp"

// ─── Sensors ─────────────────────────────────────────────────────────────────────
#include "rv1126b/sensor/imu.hpp"

// ─── System ──────────────────────────────────────────────────────────────────────
#include "rv1126b/system/sysinfo.hpp"
#include "rv1126b/system/watchdog.hpp"
#include "rv1126b/network/netconfig.hpp"
