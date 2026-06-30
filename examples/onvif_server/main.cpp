// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// onvif_server — ONVIF Profile S 服务器示例
// 启动 ONVIF 服务器（端口 8899），打印发现地址，
// 将摄像头与 RTSP 服务器关联，实现基本的 Profile S 合规性。
// 按 Ctrl+C 优雅退出。

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>
#include <rv1126b/onvif_server.hpp>
#include <rv1126b/rtsp_server.hpp>
#include <rv1126b/version.hpp>

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string>
#include <thread>

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

// 获取设备第一个非 lo 的 IPv4 地址
static std::string get_device_ip()
{
    struct ifaddrs* ifas = nullptr;
    if (getifaddrs(&ifas) != 0) return "127.0.0.1";

    std::string ip = "127.0.0.1";
    for (auto* ifa = ifas; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if (std::string(ifa->ifa_name) == "lo") continue;
        char buf[INET_ADDRSTRLEN];
        auto* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
            ip = buf;
            break;
        }
    }
    freeifaddrs(ifas);
    return ip;
}

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    std::printf("=== ONVIF Profile S 服务器示例 ===\n\n");

    // ---------------------------------------------------------------
    // 1. 打开摄像头
    // ---------------------------------------------------------------
    rv1126b::Camera::Config cam_cfg;
    cam_cfg.width        = 1920;
    cam_cfg.height       = 1080;
    cam_cfg.fps          = 25;
    cam_cfg.codec        = rv1126b::CodecType::H264;
    cam_cfg.bitrate_kbps = 4000;

    rv1126b::Camera camera;
    if (!camera.open(cam_cfg).ok()) {
        std::fprintf(stderr, "[错误] 打开摄像头失败\n");
        return EXIT_FAILURE;
    }
    std::printf("摄像头已打开 (1920×1080 H.264 25fps)\n");

    // ---------------------------------------------------------------
    // 2. 启动 RTSP 服务器（ONVIF 的媒体传输层）
    // ---------------------------------------------------------------
    rv1126b::RTSPServer rtsp;

    rv1126b::RTSPServer::Config rtsp_cfg;
    rtsp_cfg.port = 8554;
    rtsp_cfg.path   = "/live";
    rtsp_cfg.codec  = rv1126b::CodecType::H264;
    rtsp_cfg.width  = cam_cfg.width;
    rtsp_cfg.height = cam_cfg.height;
    rtsp_cfg.fps    = cam_cfg.fps;

    if (!rtsp.start(rtsp_cfg).ok()) {
        std::fprintf(stderr, "[错误] RTSP 服务器启动失败\n");
        camera.stop();
        return EXIT_FAILURE;
    }
    std::printf("RTSP 服务器已启动 (端口 554, 路径 /live)\n");

    // 注册帧回调：摄像头帧 -> RTSP 服务器
    uint64_t pts_us = 0;
    constexpr uint64_t PTS_STEP = 1000000 / 25;

    camera.set_frame_callback(
        [&rtsp, &pts_us](const rv1126b::VideoFrame& frame) {
            rtsp.push_frame(frame.data, frame.size, pts_us, frame.is_keyframe);
            pts_us += PTS_STEP;
        }
    );

    camera.start();

    // ---------------------------------------------------------------
    // 3. 配置并启动 ONVIF 服务器
    // ---------------------------------------------------------------
    std::string device_ip = get_device_ip();

    rv1126b::ONVIFServer onvif;

    rv1126b::ONVIFServer::Config onvif_cfg;
    onvif_cfg.port             = 8899;
    onvif_cfg.name             = "RV1126B-CAM";
    onvif_cfg.manufacturer     = "RV1126B SDK";
    onvif_cfg.model            = "RV1126B";
    onvif_cfg.firmware_version = RV1126B_SDK_VERSION_STRING;
    onvif_cfg.serial_number    = "RV1126B-001";
    // Profile S 媒体流信息
    onvif_cfg.rtsp_url         = "rtsp://" + device_ip + rtsp_cfg.path;
    onvif_cfg.width            = cam_cfg.width;
    onvif_cfg.height           = cam_cfg.height;
    onvif_cfg.fps              = cam_cfg.fps;

    auto onvif_start = onvif.start(onvif_cfg);
    if (!onvif_start.ok()) {
        std::fprintf(stderr, "[错误] ONVIF 服务器启动失败: %s\n",
                     std::string(rv1126b::to_string(onvif_start.error())).c_str());
        rtsp.stop();
        camera.stop();
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------------
    // 4. 打印服务地址
    // ---------------------------------------------------------------
    std::printf("\n");
    std::printf("ONVIF 设备发现地址 : http://%s:%u/onvif/device_service\n",
                device_ip.c_str(), onvif_cfg.port);
    std::printf("媒体流地址 (RTSP)  : %s\n", onvif_cfg.rtsp_url.c_str());
    std::printf("\n");
    std::printf("可用工具测试 ONVIF:\n");
    std::printf("  ONVIF Device Manager (Windows)\n");
    std::printf("  ffplay %s\n", onvif_cfg.rtsp_url.c_str());
    std::printf("\n按 Ctrl+C 停止服务...\n\n");

    // ---------------------------------------------------------------
    // 5. 运行直至退出信号
    // ---------------------------------------------------------------
    while (!g_stop.load(std::memory_order_relaxed))
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // ---------------------------------------------------------------
    // 6. 优雅停止
    // ---------------------------------------------------------------
    std::printf("\n正在停止服务...\n");
    onvif.stop();
    rtsp.stop();
    camera.stop();
    std::printf("程序退出。\n");
    return EXIT_SUCCESS;
}
