/**
 * @file camera.hpp
 * @brief RV1126B SDK — Camera module
 *
 * Wraps the V4L2 capture + RKMPP encode pipeline into a unified video/image capture interface.
 *
 * Features:
 *  - H264 / H265 / MJPEG encoded output
 *  - Multi-stream support (main stream + sub stream)
 *  - Single-frame JPEG snapshot
 *  - Frame rate / bit rate / GOP adjustable at runtime
 *
 * Usage example:
 * @code
 * Camera::Config cfg;
 * cfg.width  = 1920;
 * cfg.height = 1080;
 * cfg.fps    = 30;
 * cfg.codec  = CodecType::H264;
 * cfg.bitrate_kbps = 4000;
 *
 * Camera cam;
 * cam.open(cfg);
 * cam.set_frame_callback([](const VideoFrame& f) {
 *     // process f.data, f.size, f.is_keyframe ...
 * });
 * cam.start();
 * // ...
 * cam.stop();
 * cam.close();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include "rv1126b/core/utils.hpp"
#include <functional>
#include <string>
#include <memory>

namespace rv1126b {

class Camera {
public:
    // ─── Configuration ────────────────────────────────────────────────────────────

    struct Config {
        int         width        {1920};
        int         height       {1080};
        int         fps          {30};
        CodecType   codec        {CodecType::H264};
        int         bitrate_kbps {4000};  ///< Target bit rate (kbps)
        int         gop          {60};    ///< GOP size (frames)
        PixelFormat raw_format   {PixelFormat::NV12};
        std::string device       {"/dev/video0"};
        int         buf_count    {4};     ///< V4L2 buffer count
    };

    // ─── Lifecycle ────────────────────────────────────────────────────────────────

    Camera();
    ~Camera();

    Camera(const Camera&)            = delete;
    Camera& operator=(const Camera&) = delete;

    /// Initialize the device and configure capture parameters
    Result<void> open(const Config& config);

    /// Release the device and stop capture
    void close();

    // ─── Capture control ──────────────────────────────────────────────────────────

    /// Register encoded frame callback (fired from a dedicated thread; data lifetime is limited to the callback)
    void set_frame_callback(FrameCallback cb);

    /// Start capture
    Result<void> start();

    /// Stop capture (blocks until the capture thread exits)
    void stop();

    /// Whether capture is currently active
    bool is_running() const noexcept;

    // ─── Runtime parameter adjustment ────────────────────────────────────────────

    Result<void> set_bitrate(int kbps);
    Result<void> set_fps(int fps);
    Result<void> set_gop(int gop);

    /// Request insertion of an IDR keyframe
    Result<void> request_idr();

    // ─── Snapshot ─────────────────────────────────────────────────────────────────

    /**
     * @brief Synchronously capture one JPEG frame and save it to a file
     * @param path  Destination file path, e.g. "/tmp/snap.jpg"
     * @param quality JPEG quality 1-100
     */
    Result<void> capture_jpeg(const std::string& path, int quality = 90);

    /**
     * @brief Synchronously capture one raw NV12 frame
     * @param buf Output buffer (must be pre-allocated with width * height * 3 / 2 bytes)
     */
    Result<size_t> capture_raw(uint8_t* buf, size_t buf_size);

    // ─── Query ────────────────────────────────────────────────────────────────────

    const Config& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
