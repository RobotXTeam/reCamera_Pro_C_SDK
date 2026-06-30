// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// rtmp_push — RTMP 推流示例
// 从命令行接收推流地址，打开摄像头后连接 RTMP 服务器并推流，
// 断线后自动重连，定期打印推流统计。
//
// 用法: ./rtmp_push rtmp://server/live/stream

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>
#include <rv1126b/rtmp_pusher.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

// ---------------------------------------------------------------
// 推流统计
// ---------------------------------------------------------------
struct PushStats {
    std::mutex  mtx;
    uint64_t    frames_pushed  = 0;
    uint64_t    bytes_pushed   = 0;
    uint64_t    reconnects     = 0;
    uint64_t    frames_in_sec  = 0;
    uint64_t    bytes_in_sec   = 0;
};

int main(int argc, char* argv[])
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    // ---------------------------------------------------------------
    // 1. 解析命令行参数
    // ---------------------------------------------------------------
    if (argc < 2) {
        std::fprintf(stderr, "用法: %s <rtmp_url>\n", argv[0]);
        std::fprintf(stderr, "示例: %s rtmp://192.168.x.xx/live/stream\n", argv[0]);
        return EXIT_FAILURE;
    }

    const std::string rtmp_url = argv[1];
    std::printf("=== RTMP 推流示例 ===\n");
    std::printf("推流地址: %s\n\n", rtmp_url.c_str());

    // ---------------------------------------------------------------
    // 2. 打开摄像头（1280×720，2Mbps，30fps）
    // ---------------------------------------------------------------
    rv1126b::Camera::Config cam_cfg;
    cam_cfg.width        = 1280;
    cam_cfg.height       = 720;
    cam_cfg.fps          = 30;
    cam_cfg.codec        = rv1126b::CodecType::H264;
    cam_cfg.bitrate_kbps = 2000;

    rv1126b::Camera camera;
    if (!camera.open(cam_cfg).ok()) {
        std::fprintf(stderr, "[错误] 打开摄像头失败\n");
        return EXIT_FAILURE;
    }
    std::printf("摄像头已打开 (1280×720 H.264 30fps 2Mbps)\n");

    // ---------------------------------------------------------------
    // 3. 初始连接 RTMP 服务器
    // ---------------------------------------------------------------
    rv1126b::RTMPPusher rtmp;
    PushStats           stats;

    auto connect_rtmp = [&]() -> bool {
        rv1126b::RTMPPusher::Config rtmp_cfg;
        rtmp_cfg.url          = rtmp_url;
        rtmp_cfg.width        = cam_cfg.width;
        rtmp_cfg.height       = cam_cfg.height;
        rtmp_cfg.fps          = cam_cfg.fps;
        rtmp_cfg.bitrate_kbps = cam_cfg.bitrate_kbps;
        rtmp_cfg.codec        = rv1126b::CodecType::H264;
        auto res = rtmp.connect(rtmp_cfg);
        if (!res.ok()) {
            std::fprintf(stderr, "[错误] RTMP 连接失败: %s\n",
                         std::string(rv1126b::to_string(res.error())).c_str());
            return false;
        }
        std::printf("[RTMP] 已连接: %s\n", rtmp_url.c_str());
        return true;
    };

    if (!connect_rtmp()) {
        camera.stop();
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------------
    // 4. 注册帧回调，推送摄像头帧
    // ---------------------------------------------------------------
    std::atomic<bool>   connected{true};
    uint64_t            pts_us   = 0;
    constexpr uint64_t  PTS_STEP = 1000000 / 30;

    camera.set_frame_callback(
        [&](const rv1126b::VideoFrame& frame) {
            if (g_stop.load()) return;

            // 如果当前断开则跳过
            if (!connected.load(std::memory_order_relaxed)) return;

            auto res = rtmp.push_frame(frame.data, frame.size,
                                       pts_us, frame.is_keyframe);
            pts_us += PTS_STEP;

            if (!res.ok()) {
                // 推送失败，标记为断线，由重连线程处理
                connected.store(false, std::memory_order_relaxed);
                return;
            }

            std::lock_guard<std::mutex> lk(stats.mtx);
            stats.frames_pushed++;
            stats.bytes_pushed  += frame.size;
            stats.frames_in_sec++;
            stats.bytes_in_sec  += frame.size;
        }
    );

    if (!camera.start().ok()) {
        std::fprintf(stderr, "[错误] 摄像头启动失败\n");
        rtmp.disconnect();
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------------
    // 5. 主循环：监控连接、打印统计，断线时自动重连
    // ---------------------------------------------------------------
    std::printf("推流中，按 Ctrl+C 停止...\n\n");
    std::printf("%-8s  %-8s  %-10s  %-10s  %-10s\n",
                "时间(s)", "FPS", "码率(kbps)", "总帧数", "重连次数");
    std::printf("%s\n", std::string(55, '-').c_str());

    auto t_start = std::chrono::steady_clock::now();
    auto t_last  = t_start;

    while (!g_stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto now     = std::chrono::steady_clock::now();
        double secs  = std::chrono::duration<double>(now - t_start).count();
        double dt    = std::chrono::duration<double>(now - t_last).count();
        t_last = now;

        // 打印统计
        {
            std::lock_guard<std::mutex> lk(stats.mtx);
            double fps_now     = dt > 0 ? stats.frames_in_sec / dt : 0.0;
            double bitrate_kbps = dt > 0 ? stats.bytes_in_sec * 8.0 / dt / 1000.0 : 0.0;

            std::printf("%-8.1f  %-8.1f  %-10.0f  %-10llu  %-10llu\n",
                        secs, fps_now, bitrate_kbps,
                        (unsigned long long)stats.frames_pushed,
                        (unsigned long long)stats.reconnects);

            stats.frames_in_sec = 0;
            stats.bytes_in_sec  = 0;
        }

        // 自动重连
        if (!connected.load(std::memory_order_relaxed) && !g_stop.load()) {
            std::printf("[RTMP] 连接断开，3 秒后尝试重连...\n");
            rtmp.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(3));

            if (connect_rtmp()) {
                connected.store(true, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lk(stats.mtx);
                stats.reconnects++;
            }
        }
    }

    // ---------------------------------------------------------------
    // 6. 清理
    // ---------------------------------------------------------------
    camera.stop();
    rtmp.disconnect();

    std::lock_guard<std::mutex> lk(stats.mtx);
    std::printf("\n=== 推流汇总 ===\n");
    std::printf("总推送帧数 : %llu\n",  (unsigned long long)stats.frames_pushed);
    std::printf("总数据量   : %.2f MB\n", stats.bytes_pushed / (1024.0 * 1024.0));
    std::printf("重连次数   : %llu\n",  (unsigned long long)stats.reconnects);
    std::printf("程序退出。\n");
    return EXIT_SUCCESS;
}
