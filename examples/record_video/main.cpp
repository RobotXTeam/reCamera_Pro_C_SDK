// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// record_video — 视频录制示例
// 以 1920×1080 H.264 30fps 打开摄像头，将码流直接写入
// /tmp/output.h264（原始 Annex-B 码流，无容器）。
// 录制 10 秒后自动退出，或按 Ctrl+C 提前结束。

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

// ---------------------------------------------------------------
// 录制统计
// ---------------------------------------------------------------
struct RecordStats {
    std::mutex  mtx;
    uint64_t    total_frames   = 0;
    uint64_t    keyframes      = 0;
    uint64_t    total_bytes    = 0;
    uint64_t    frames_in_sec  = 0;
};

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    constexpr const char* OUTPUT_FILE = "/tmp/output.h264";
    std::printf("=== 视频录制示例 ===\n");
    std::printf("分辨率: 1920×1080  编码: H.264  帧率: 30fps  码率: 8Mbps\n");
    std::printf("输出文件: %s\n\n", OUTPUT_FILE);

    // ---------------------------------------------------------------
    // 1. 打开输出文件
    // ---------------------------------------------------------------
    std::ofstream ofs(OUTPUT_FILE, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::fprintf(stderr, "[错误] 无法创建输出文件: %s\n", OUTPUT_FILE);
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------------
    // 2. 配置并打开摄像头
    // ---------------------------------------------------------------
    rv1126b::Camera::Config cfg;
    cfg.width        = 1920;
    cfg.height       = 1080;
    cfg.fps          = 30;
    cfg.codec        = rv1126b::CodecType::H264;
    cfg.bitrate_kbps = 8000;

    rv1126b::Camera camera;
    auto open_res = camera.open(cfg);
    if (!open_res.ok()) {
        std::fprintf(stderr, "[错误] 打开摄像头失败: %s\n",
                     std::string(rv1126b::to_string(open_res.error())).c_str());
        return EXIT_FAILURE;
    }

    // ---------------------------------------------------------------
    // 3. 注册帧回调：将每帧 H.264 数据追加写入文件
    // ---------------------------------------------------------------
    RecordStats stats;

    camera.set_frame_callback(
        [&ofs, &stats](const rv1126b::VideoFrame& frame) {
            // 直接写入原始 Annex-B 数据
            ofs.write(reinterpret_cast<const char*>(frame.data), frame.size);

            std::lock_guard<std::mutex> lk(stats.mtx);
            stats.total_frames++;
            stats.frames_in_sec++;
            stats.total_bytes += frame.size;
            if (frame.is_keyframe) stats.keyframes++;
        }
    );

    // ---------------------------------------------------------------
    // 4. 启动录制
    // ---------------------------------------------------------------
    auto start_res = camera.start();
    if (!start_res.ok()) {
        std::fprintf(stderr, "[错误] 摄像头启动失败: %s\n",
                     std::string(rv1126b::to_string(start_res.error())).c_str());
        return EXIT_FAILURE;
    }

    constexpr auto REC_DURATION = std::chrono::seconds(10);
    auto rec_start = std::chrono::steady_clock::now();

    std::printf("录制中，持续 10 秒（或按 Ctrl+C 提前停止）...\n\n");

    // ---------------------------------------------------------------
    // 5. 主循环：每秒打印进度
    // ---------------------------------------------------------------
    while (!g_stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto elapsed = std::chrono::steady_clock::now() - rec_start;
        if (elapsed >= REC_DURATION) break;

        double secs = std::chrono::duration<double>(elapsed).count();
        std::lock_guard<std::mutex> lk(stats.mtx);
        double bitrate_kbps = stats.frames_in_sec > 0
            ? static_cast<double>(stats.total_bytes) / secs * 8.0 / 1000.0
            : 0.0;
        std::printf("[%4.1fs] 帧数: %5llu  关键帧: %4llu  数据量: %.2f MB  码率: %.0f kbps\n",
                    secs,
                    (unsigned long long)stats.total_frames,
                    (unsigned long long)stats.keyframes,
                    static_cast<double>(stats.total_bytes) / (1024.0 * 1024.0),
                    bitrate_kbps);
        stats.frames_in_sec = 0;
    }

    // ---------------------------------------------------------------
    // 6. 停止并汇总
    // ---------------------------------------------------------------
    camera.stop();
    ofs.flush();
    ofs.close();

    double total_secs =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - rec_start).count();

    std::printf("\n=== 录制完成 ===\n");
    std::printf("录制时长   : %.1f 秒\n", total_secs);
    std::printf("总帧数     : %llu\n",
                (unsigned long long)stats.total_frames);
    std::printf("总数据量   : %.2f MB\n",
                static_cast<double>(stats.total_bytes) / (1024.0 * 1024.0));
    std::printf("平均码率   : %.0f kbps\n",
                total_secs > 0
                ? stats.total_bytes * 8.0 / total_secs / 1000.0
                : 0.0);
    std::printf("输出文件   : %s\n", OUTPUT_FILE);
    std::printf("\n播放命令: ffplay -f h264 %s\n", OUTPUT_FILE);
    std::printf("或转封装: ffmpeg -f h264 -i %s -c copy /tmp/output.mp4\n", OUTPUT_FILE);

    return EXIT_SUCCESS;
}
