/**
 * @file onvif.hpp
 * @brief RV1126B SDK — ONVIF Profile S server (lightweight implementation)
 *
 * Implements the minimum ONVIF Profile S Web Service interface:
 *  - GetSystemDateAndTime
 *  - GetCapabilities
 *  - GetProfiles
 *  - GetStreamUri   → returns the internal RTSP URL
 *  - GetSnapshotUri
 *  - PTZ (stub; returns "not supported")
 *
 * Implements HTTP/1.1 SOAP service over BSD sockets; no gSOAP or external library needed.
 * WS-Discovery multicast discovery (239.255.255.250:3702) is optionally enabled.
 *
 * @code
 * ONVIFServer onvif;
 * ONVIFServer::Config cfg;
 * cfg.port        = 8899;
 * cfg.name        = "RV1126B-Camera";
 * cfg.location    = "Server Room";
 * cfg.rtsp_url    = "rtsp://192.168.x.xx:554/live";
 * cfg.snapshot_url = "http://192.168.x.xx:8899/snapshot.jpg";
 * onvif.start(cfg);
 * // ...
 * onvif.stop();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <memory>
#include <string>

namespace rv1126b {

class ONVIFServer {
public:
    struct Config {
        int         port            {8899};
        std::string name            {"RV1126B Camera"};
        std::string location        {"Unknown"};
        std::string manufacturer    {"Rockchip"};
        std::string model           {"RV1126B"};
        std::string firmware_version{"1.0.0"};
        std::string serial_number   {"000000000001"};
        std::string hardware_id     {"RV1126B-01"};

        /// RTSP stream URL (returned by GetStreamUri)
        std::string rtsp_url        ;
        /// Snapshot URL (returned by GetSnapshotUri)
        std::string snapshot_url    {};

        /// Video resolution (used in Profile description)
        int         width           {1920};
        int         height          {1080};
        int         fps             {30};

        /// Whether to enable WS-Discovery multicast (requires root to open multicast socket)
        bool        enable_discovery{false};
    };

    ONVIFServer();
    ~ONVIFServer();

    ONVIFServer(const ONVIFServer&)            = delete;
    ONVIFServer& operator=(const ONVIFServer&) = delete;

    /**
     * @brief Start the ONVIF HTTP server
     * Listens on TCP port, handles SOAP requests, optionally starts WS-Discovery.
     */
    Result<void> start(const Config& config);

    /// Stop the server
    void stop();

    bool is_running() const noexcept;

    /**
     * @brief Dynamically update the RTSP URL (call when the stream address changes)
     */
    void set_rtsp_url(const std::string& url);

    /**
     * @brief Dynamically update the snapshot URL
     */
    void set_snapshot_url(const std::string& url);

    const Config& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
