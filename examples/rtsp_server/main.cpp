// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// rtsp_server — RTSP 流媒体服务器示例
// 打开摄像头，启动 RTSP 服务器，将 H.264 码流推送到 /live 路径，
// 每 5 秒打印一次客户端连接数。

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>
#include <rv1126b/rtsp_server.hpp>

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ifaddrs.h>
#include <mutex>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <thread>

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

// 获取设备的第一个非 lo IPv4 地址
static std::string get_device_ip()
{
    struct ifaddrs* ifas = nullptr;
    if (getifaddrs(&ifas) != 0) return "127.0.0.1";

    std::string result = "127.0.0.1";
    for (auto* ifa = ifas; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        if (std::string(ifa->ifa_name) == "lo") continue;

        char buf[INET_ADDRSTRLEN];
        auto* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
            result = buf;
            break;
        }
    }
    freeifaddrs(ifas);
    return result;
}

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    std::printf("=== RTSP 流媒体服务器示例 ===\n\n");

    // ---------------------------------------------------------------
    // 1. 配置并打开摄像头
    // ---------------------------------------------------------------
    rv1126b::Camera::Config cam_cfg;
    cam_cfg.width        = 1920;
    cam_cfg.height       = 1080;
    cam_cfg.fps          = 30;
    cam_cfg.codec        = rv1126b::CodecType::H264;
    cam_cfg.bitrate_kbps = 4000;

    rv1126b::Camera camera;
    if (!camera.open(cam_cfg).ok()) {
        std::fprintf(stderr, "[错误] 打开摄像头失败\n");
        return EXIT_FAILURE;
    }
    std::printf("摄像头已打开 (1920×1080 H.264 30fps)\n");

    // ---------------------------------------------------------------
    // 2. 启动 RTSP 服务器
    // ---------------------------------------------------------------
    rv1126b::RTSPServer rtsp;

    rv1126b::RTSPServer::Config rtsp_cfg;
    rtsp_cfg.port      = 8554;   // 554 通常被 go2rtc 占用，使用 8554
    rtsp_cfg.path      = "/live";
    rtsp_cfg.codec     = rv1126b::CodecType::H264;
    rtsp_cfg.width     = cam_cfg.width;
    rtsp_cfg.height    = cam_cfg.height;
    rtsp_cfg.fps       = cam_cfg.fps;

    auto start_res = rtsp.start(rtsp_cfg);
    if (!start_res.ok()) {
        std::fprintf(stderr, "[错误] RTSP 服务器启动失败: %s\n",
                     std::string(rv1126b::to_string(start_res.error())).c_str());
        camera.stop();
        return EXIT_FAILURE;
    }

    std::string device_ip = get_device_ip();
    std::printf("\n流地址: rtsp://%s%s\n", device_ip.c_str(), rtsp_cfg.path.c_str());
    std::printf("播放命令: ffplay rtsp://%s%s\n\n", device_ip.c_str(), rtsp_cfg.path.c_str());

    // ---------------------------------------------------------------
    // 3. 注册帧回调，将摄像头帧推送给 RTSP 服务器
    // ---------------------------------------------------------------
    std::mutex  stats_mtx;
    uint64_t    frame_count = 0;
    uint64_t    pts_us      = 0;
    constexpr uint64_t PTS_STEP_US = 1000000 / 30;  // 30fps -> ~33333 µs

    camera.set_frame_callback(
        [&](const rv1126b::VideoFrame& frame) {
            // 将帧推给 RTSP 服务器
            rtsp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
            pts_us += PTS_STEP_US;

            std::lock_guard<std::mutex> lk(stats_mtx);
            frame_count++;
        }
    );

    if (!camera.start().ok()) {
        std::fprintf(stderr, "[错误] 摄像头启动失败\n");
        rtsp.stop();
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------------
    // 4. 主循环：每 5 秒打印客户端连接数
    // ---------------------------------------------------------------
    std::printf("RTSP 服务器运行中，按 Ctrl+C 停止...\n\n");
    auto last_report = std::chrono::steady_clock::now();

    while (!g_stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto now = std::chrono::steady_clock::now();
        if (now - last_report >= std::chrono::seconds(5)) {
            last_report = now;
            int clients = rtsp.client_count();
            std::lock_guard<std::mutex> lk(stats_mtx);
            std::printf("[状态] 已推送: %llu 帧  当前连接客户端: %d\n",
                        (unsigned long long)frame_count, clients);
        }
    }

    // ---------------------------------------------------------------
    // 5. 清理
    // ---------------------------------------------------------------
    camera.stop();
    rtsp.stop();
    std::printf("\nRTSP 服务器已停止。\n");
    return EXIT_SUCCESS;
}
