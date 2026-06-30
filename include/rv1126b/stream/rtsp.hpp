/**
 * @file rtsp.hpp
 * @brief RV1126B SDK — RTSP server module
 *
 * Lightweight RTSP/1.0 server implemented over BSD sockets.
 * Supports H.264 RTP packetization (single NAL and FU-A fragmentation).
 * Supports both TCP (RTP over RTSP interleaved) and UDP transport.
 *
 * URL format: rtsp://<device-ip>:<port><path>
 *
 * @code
 * RTSPServer rtsp;
 * RTSPServer::Config cfg;
 * cfg.port  = 554;
 * cfg.path  = "/live";
 * cfg.codec = CodecType::H264;
 * cfg.width = 1920;
 * cfg.height = 1080;
 * cfg.fps   = 30;
 * rtsp.start(cfg);
 *
 * // push frames from encoder callback
 * rtsp.push_frame(pkt.data, pkt.size, pkt.pts_us, pkt.is_keyframe);
 *
 * rtsp.stop();
 * @endcode
 */
#pragma once

#include <memory>

#include "rv1126b/core/error.hpp"
#include "rv1126b/core/utils.hpp"
#include <cstdint>
#include <string>

namespace rv1126b {

class RTSPServer {
public:
    struct Config {
        int         port        {554};
        std::string path        {"/live"};
        CodecType   codec       {CodecType::H264};
        int         width       {1920};
        int         height      {1080};
        int         fps         {30};
        int         max_clients {4};      ///< Maximum concurrent client count
        bool        tcp_only    {false};  ///< true=TCP only, false=TCP+UDP
    };

    RTSPServer();
    ~RTSPServer();

    RTSPServer(const RTSPServer&)            = delete;
    RTSPServer& operator=(const RTSPServer&) = delete;

    /**
     * @brief Start the RTSP server (create listen socket, start accept thread)
     */
    Result<void> start(const Config& config);

    /// Stop the server and disconnect all clients
    void stop();

    bool is_running() const noexcept;

    /**
     * @brief Push one encoded data frame
     * @param data          Encoded data (H264 Annex-B or AVCC; SDK handles both)
     * @param size          Data byte count
     * @param pts_us        Presentation timestamp (microseconds)
     * @param is_keyframe   Whether this is a keyframe (IDR)
     */
    Result<void> push_frame(const uint8_t* data, size_t size,
                            int64_t pts_us, bool is_keyframe);

    /// Number of currently connected clients
    int client_count() const noexcept;

    /// Full RTSP URL (e.g. rtsp://0.0.0.0:554/live)
    std::string url() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
