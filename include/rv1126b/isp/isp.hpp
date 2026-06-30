/**
 * @file isp.hpp
 * @brief RV1126B SDK — ISP (Image Signal Processor) module
 *
 * Wraps the RockChip AIQ (rkaiq) ISP control interface, providing a unified API for image parameter tuning:
 *  - Exposure control (auto/manual)
 *  - White balance (auto/manual color temperature)
 *  - Gain (analog/digital gain)
 *  - Noise reduction (spatial/temporal)
 *  - HDR mode
 *  - Color enhancement (saturation/sharpness/contrast)
 *  - Mirror/flip
 *
 * @code
 * ISP isp;
 * isp.init("/etc/iqfiles");
 *
 * // Set manual exposure
 * ISP::ExposureConfig exp;
 * exp.mode = ISP::ExposureMode::Manual;
 * exp.time_ms = 10.0f;
 * exp.iso = 200;
 * isp.set_exposure(exp);
 *
 * // Query current exposure info
 * auto info = isp.get_exposure_info();
 * printf("current time=%.2f ms, gain=%.2f\n",
 *        info->time_ms, info->analog_gain);
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <memory>
#include <string>

namespace rv1126b {

class ISP {
public:
    // ─── Exposure ─────────────────────────────────────────────────────────────────

    enum class ExposureMode {
        Auto   = 0,
        Manual = 1,
        Shutter,    ///< Shutter priority (fixed shutter speed)
        Iso,        ///< ISO priority
    };

    struct ExposureConfig {
        ExposureMode mode    {ExposureMode::Auto};
        float time_ms        {0.0f};   ///< Exposure time (ms, valid in Manual mode)
        int   iso            {0};      ///< ISO value (valid in Manual/Iso modes)
        float ev_bias        {0.0f};   ///< EV bias (Auto mode ±3 EV)
    };

    struct ExposureInfo {
        float time_ms;         ///< Current exposure time (ms)
        float analog_gain;     ///< Current analog gain
        float digital_gain;    ///< Current digital gain
        int   iso;             ///< Equivalent ISO
    };

    // ─── White balance ────────────────────────────────────────────────────────────

    enum class WBMode {
        Auto      = 0,
        Manual    = 1,  ///< Manual R/G/B gain
        ColorTemp = 2,  ///< Specify color temperature (K)
    };

    struct WhiteBalanceConfig {
        WBMode  mode        {WBMode::Auto};
        int     color_temp  {5500};   ///< Color temperature (K), ColorTemp mode
        float   r_gain      {1.0f};   ///< R gain, Manual mode
        float   g_gain      {1.0f};   ///< G gain
        float   b_gain      {1.0f};   ///< B gain
    };

    // ─── Noise reduction ──────────────────────────────────────────────────────────

    struct NoiseReductionConfig {
        bool  enable_spatial  {true};    ///< Spatial noise reduction
        bool  enable_temporal {true};    ///< Temporal noise reduction (3DNR)
        int   strength        {50};      ///< Noise reduction strength 0-100
    };

    // ─── Color enhancement ────────────────────────────────────────────────────────

    struct ColorConfig {
        int saturation {50};    ///< Saturation  0-100 (50=normal)
        int sharpness  {50};    ///< Sharpness   0-100
        int contrast   {50};    ///< Contrast    0-100
        int brightness {50};    ///< Brightness  0-100
    };

    // ─── HDR ───────────────────────────────────────────────────────────────────

    enum class HDRMode {
        Off    = 0,
        HDR2x  = 1,   ///< 2-frame HDR
        HDR3x  = 2,   ///< 3-frame HDR
        LLHDR  = 3,   ///< Low-light HDR
    };

    // ─── Lifecycle ────────────────────────────────────────────────────────────────

    ISP();
    ~ISP();

    ISP(const ISP&)            = delete;
    ISP& operator=(const ISP&) = delete;

    /**
     * @brief Initialize the ISP
     * @param iq_dir IQ parameter file directory (e.g. "/etc/iqfiles")
     */
    Result<void> init(const std::string& iq_dir = "/etc/iqfiles");

    /// Release ISP resources
    void deinit();

    bool is_initialized() const noexcept;

    // ─── Exposure control ─────────────────────────────────────────────────────────

    Result<void> set_exposure(const ExposureConfig& config);
    Result<ExposureInfo> get_exposure_info() const;

    // ─── White balance ────────────────────────────────────────────────────────────

    Result<void> set_white_balance(const WhiteBalanceConfig& config);
    Result<WhiteBalanceConfig> get_white_balance() const;

    // ─── Gain ─────────────────────────────────────────────────────────────────────

    Result<void> set_analog_gain(float gain);
    Result<void> set_digital_gain(float gain);

    // ─── Noise reduction ──────────────────────────────────────────────────────────

    Result<void> set_noise_reduction(const NoiseReductionConfig& config);

    // ─── HDR ───────────────────────────────────────────────────────────────────

    Result<void> set_hdr_mode(HDRMode mode);
    HDRMode      get_hdr_mode() const noexcept;

    // ─── Color enhancement ────────────────────────────────────────────────────────

    Result<void> set_color(const ColorConfig& config);
    Result<ColorConfig> get_color() const;

    // ─── Mirror/flip ──────────────────────────────────────────────────────────────

    Result<void> set_mirror(bool horizontal, bool vertical);

    // ─── IQ file hot-reload ──────────────────────────────────────────────────────────

    /// Reload IQ parameter file without restarting the pipeline
    Result<void> reload_iq(const std::string& iq_file);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
