// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// camera_preview — 摄像头预览示例
// 以 1920×1080 H.264 30fps 打开摄像头，通过帧回调统计帧率，
// 运行 30 秒后自动退出（或按 Ctrl+C 提前退出）。

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>

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
// 帧统计
// ---------------------------------------------------------------
struct FrameStats {
    std::mutex  mtx;
    uint64_t    total_frames   = 0;
    uint64_t    keyframe_count = 0;
    uint64_t    total_bytes    = 0;
    uint64_t    frames_in_sec  = 0;   // 当前秒内帧数（用于 FPS 计算）
    std::chrono::steady_clock::time_point last_fps_time;
};

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    std::printf("=== 摄像头预览示例 ===\n");
    std::printf("分辨率: 1920×1080  编码: H.264  帧率: 30fps  码率: 4Mbps\n\n");

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
    auto open_res = camera.open(cam_cfg);
    if (!open_res.ok()) {
        std::fprintf(stderr, "[错误] 打开摄像头失败: %s\n",
                     std::string(rv1126b::to_string(open_res.error())).c_str());
        return EXIT_FAILURE;
    }
    std::printf("摄像头已打开\n");

    // ---------------------------------------------------------------
    // 2. 注册帧回调
    // ---------------------------------------------------------------
    FrameStats stats;
    stats.last_fps_time = std::chrono::steady_clock::now();

    camera.set_frame_callback(
        [&stats](const rv1126b::VideoFrame& frame) {
            std::lock_guard<std::mutex> lk(stats.mtx);
            stats.total_frames++;
            stats.frames_in_sec++;
            stats.total_bytes   += frame.size;
            if (frame.is_keyframe) stats.keyframe_count++;
        }
    );

    // ---------------------------------------------------------------
    // 3. 开始采集
    // ---------------------------------------------------------------
    auto start_res = camera.start();
    if (!start_res.ok()) {
        std::fprintf(stderr, "[错误] 摄像头启动失败: %s\n",
                     std::string(rv1126b::to_string(start_res.error())).c_str());
        camera.stop();
        return EXIT_FAILURE;
    }
    std::printf("摄像头已启动，运行 30 秒（或按 Ctrl+C 退出）...\n\n");

    // ---------------------------------------------------------------
    // 4. 主循环：每秒打印一次 FPS
    // ---------------------------------------------------------------
    constexpr auto  RUN_DURATION = std::chrono::seconds(30);
    auto            run_start    = std::chrono::steady_clock::now();

    while (!g_stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto now     = std::chrono::steady_clock::now();
        auto elapsed = now - run_start;

        {
            std::lock_guard<std::mutex> lk(stats.mtx);
            double fps = static_cast<double>(stats.frames_in_sec);
            std::printf("[%5.1fs] FPS: %5.1f  总帧数: %6llu  关键帧: %5llu\n",
                        std::chrono::duration<double>(elapsed).count(),
                        fps,
                        (unsigned long long)stats.total_frames,
                        (unsigned long long)stats.keyframe_count);
            stats.frames_in_sec = 0;
        }

        if (elapsed >= RUN_DURATION) break;
    }

    // ---------------------------------------------------------------
    // 5. 停止并打印统计
    // ---------------------------------------------------------------
    camera.stop();

    std::lock_guard<std::mutex> lk(stats.mtx);
    double avg_size_kb = stats.total_frames > 0
        ? static_cast<double>(stats.total_bytes) / stats.total_frames / 1024.0
        : 0.0;

    std::printf("\n=== 帧统计 ===\n");
    std::printf("总帧数     : %llu\n", (unsigned long long)stats.total_frames);
    std::printf("关键帧数   : %llu\n", (unsigned long long)stats.keyframe_count);
    std::printf("总数据量   : %.2f MB\n",
                static_cast<double>(stats.total_bytes) / (1024.0 * 1024.0));
    std::printf("平均帧大小 : %.2f KB\n", avg_size_kb);
    std::printf("\n程序退出。\n");

    return EXIT_SUCCESS;
}
