// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// capture_image — 图像抓拍示例
// 打开摄像头，抓取 JPEG 快照到 /tmp/snapshot.jpg，
// 同时演示原始 NV12 帧的获取方式，然后保存第二张快照验证摄像头工作正常。

#include <rv1126b/camera.hpp>
#include <rv1126b/logger.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

// 查询文件大小（字节）
static long file_size(const char* path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    return f.is_open() ? static_cast<long>(f.tellg()) : -1L;
}

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    std::printf("=== 图像抓拍示例 ===\n\n");

    // ---------------------------------------------------------------
    // 1. 打开摄像头（1280×720，用于抓拍；不需要高码率）
    // ---------------------------------------------------------------
    rv1126b::Camera::Config cfg;
    cfg.width        = 1280;
    cfg.height       = 720;
    cfg.fps          = 15;
    cfg.codec        = rv1126b::CodecType::MJPEG;
    cfg.bitrate_kbps = 2000;

    rv1126b::Camera camera;
    auto open_res = camera.open(cfg);
    if (!open_res.ok()) {
        std::fprintf(stderr, "[错误] 打开摄像头失败: %s\n",
                     std::string(rv1126b::to_string(open_res.error())).c_str());
        return EXIT_FAILURE;
    }
    std::printf("摄像头已打开 (%dx%d)\n", cfg.width, cfg.height);

    // ---------------------------------------------------------------
    // 2. 抓取第一张 JPEG 快照
    // ---------------------------------------------------------------
    constexpr const char* SNAP1 = "/tmp/snapshot.jpg";
    std::printf("正在抓取第一张快照 -> %s ...\n", SNAP1);

    auto snap_res = camera.capture_jpeg(SNAP1);
    if (!snap_res.ok()) {
        std::fprintf(stderr, "[错误] JPEG 抓拍失败: %s\n",
                     std::string(rv1126b::to_string(snap_res.error())).c_str());
        camera.stop();
        return EXIT_FAILURE;
    }

    long sz1 = file_size(SNAP1);
    std::printf("快照 1 已保存: %s  (%.1f KB)\n\n", SNAP1, sz1 / 1024.0);

    // ---------------------------------------------------------------
    // 3. 演示原始 NV12 帧抓取
    //    NV12 格式：Y 平面 (width×height) + UV 交错平面 (width×height/2)
    // ---------------------------------------------------------------
    if (!g_stop.load()) {
        std::printf("正在抓取原始 NV12 帧...\n");

        // 启动摄像头以获取回调帧
        // WORKAROUND: 关闭并重新打开摄像头，避免前一次 capture_jpeg 导致的 VIDIOC_STREAMON 失败
        camera.close();
        if (!camera.open(cfg).ok()) {
            std::fprintf(stderr, "[错误] 重新打开摄像头失败\n");
        }

        auto start_res = camera.start();
        if (!start_res.ok()) {
            std::fprintf(stderr, "[警告] 摄像头启动失败，跳过 NV12 演示\n");
        } else {
            std::atomic<bool> got_frame{false};

            camera.set_frame_callback(
                [&got_frame, &cfg](const rv1126b::VideoFrame& frame) {
                    if (got_frame.load()) return;

                    // 仅处理第一帧 NV12 原始数据
                    if (frame.pixel_format == rv1126b::PixelFormat::NV12) {
                        constexpr const char* NV12_PATH = "/tmp/snapshot.nv12";
                        std::ofstream ofs(NV12_PATH, std::ios::binary);
                        if (ofs.is_open()) {
                            ofs.write(reinterpret_cast<const char*>(frame.data), frame.size);
                            std::printf("NV12 原始帧已保存: %s (%zu 字节)\n",
                                        NV12_PATH, frame.size);
                            std::printf("  可用 ffmpeg 转换: ffmpeg -s %dx%d -pix_fmt nv12 "
                                        "-i /tmp/snapshot.nv12 /tmp/snapshot_nv12.png\n",
                                        cfg.width, cfg.height);
                        }
                        got_frame.store(true);
                    }
                }
            );

            // 等待最多 3 秒取到一帧
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
            while (!got_frame.load() && std::chrono::steady_clock::now() < deadline
                   && !g_stop.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            camera.stop();

            if (!got_frame.load())
                std::printf("[警告] 未收到 NV12 帧（摄像头可能不支持直接输出 NV12）\n");
        }
    }

    // ---------------------------------------------------------------
    // 4. 抓取第二张 JPEG 快照，验证摄像头仍然工作
    // ---------------------------------------------------------------
    if (!g_stop.load()) {
        constexpr const char* SNAP2 = "/tmp/snapshot2.jpg";
        std::printf("\n正在抓取第二张快照 -> %s ...\n", SNAP2);

        // WORKAROUND: 关闭并重新打开摄像头
        camera.close();
        if (!camera.open(cfg).ok()) {
            std::fprintf(stderr, "[错误] 重新打开摄像头失败\n");
        }

        auto snap2_res = camera.capture_jpeg(SNAP2);
        if (snap2_res.ok()) {
            long sz2 = file_size(SNAP2);
            std::printf("快照 2 已保存: %s  (%.1f KB)\n", SNAP2, sz2 / 1024.0);
        } else {
            std::fprintf(stderr, "[警告] 第二张快照失败: %s\n",
                         std::string(rv1126b::to_string(snap2_res.error())).c_str());
        }
    }

    // ---------------------------------------------------------------
    // 5. 关闭摄像头
    // ---------------------------------------------------------------
    camera.stop();

    std::printf("\n程序退出。\n");
    return EXIT_SUCCESS;
}
