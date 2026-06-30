/**
 * @file rtmp.hpp
 * @brief RV1126B SDK — RTMP push streaming module
 *
 * Wraps librtmp to push H.264 encoded frames to an RTMP server (e.g. nginx-rtmp, SRS).
 * librtmp is expected at /usr/lib/librtmp.so (system library).
 *
 * @code
 * RTMPPusher rtmp;
 * RTMPPusher::Config cfg;
 * cfg.url    = "rtmp://192.168.x.xx/live";
 * cfg.stream = "stream0";
 * cfg.width  = 1920;
 * cfg.height = 1080;
 * cfg.fps    = 30;
 * rtmp.connect(cfg);
 *
 * rtmp.push_frame(pkt.data, pkt.size, pkt.pts_us, pkt.is_keyframe);
 *
 * rtmp.disconnect();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include "rv1126b/core/utils.hpp"
#include "rv1126b/video/encoder.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace rv1126b {

class RTMPPusher {
public:
    struct Config {
        std::string url         {};            ///< RTMP URL, e.g. "rtmp://host/app"
        std::string stream      {"stream0"};   ///< Stream name (stream key)
        int         width       {1920};
        int         height      {1080};
        int         fps         {30};
        int         bitrate_kbps{4000};
        CodecType   codec       {CodecType::H264};
        int         connect_timeout_sec{5};
        bool        auto_reconnect{true};      ///< Auto-reconnect on disconnect
        int         reconnect_interval_sec{3};
    };

    RTMPPusher();
    ~RTMPPusher();

    RTMPPusher(const RTMPPusher&)            = delete;
    RTMPPusher& operator=(const RTMPPusher&) = delete;

    /**
     * @brief Connect to RTMP server and start a push session
     * @param config Push streaming configuration
     */
    Result<void> connect(const Config& config);

    /// Disconnect and stop streaming
    void disconnect();

    bool is_connected() const noexcept;

    /**
     * @brief Push one H.264 data frame
     * @param data         H.264 frame data in Annex-B format
     * @param size         Data byte count
     * @param pts_us       Presentation timestamp (microseconds)
     * @param is_keyframe  Whether this is an IDR frame
     */
    Result<void> push_frame(const uint8_t* data, size_t size,
                            int64_t pts_us, bool is_keyframe);

    /**
     * @brief Push an encoded packet from an Encoder
     */
    Result<void> push_packet(const Encoder::Packet& pkt);

    /// Send SPS/PPS (call after connect and before pushing the first frame)
    Result<void> send_sequence_header(const uint8_t* sps, size_t sps_len,
                                      const uint8_t* pps, size_t pps_len);

    /// Number of frames pushed so far
    uint64_t frames_sent() const noexcept;

    /// Number of bytes pushed so far
    uint64_t bytes_sent() const noexcept;

    const Config& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
