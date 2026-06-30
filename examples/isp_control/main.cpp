// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// isp_control — ISP 图像信号处理控制示例
// 演示曝光、白平衡、色彩、降噪和 HDR 模式的手动控制，
// 等待 3 秒后恢复自动模式。

#include <rv1126b/camera.hpp>
#include <rv1126b/isp.hpp>
#include <rv1126b/logger.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    constexpr const char* IQ_DIR = "/oem/usr/share/iqfiles";

    std::printf("=== ISP 控制示例 ===\n");
    std::printf("IQ 文件目录: %s\n\n", IQ_DIR);

    // ---------------------------------------------------------------
    // 1. 初始化 ISP
    // ---------------------------------------------------------------
    rv1126b::ISP isp;
    auto init_res = isp.init(IQ_DIR);
    if (!init_res.ok()) {
        std::fprintf(stderr, "[错误] ISP 初始化失败: %s\n"
                     "       请确认 IQ 文件目录存在: %s\n",
                     std::string(rv1126b::to_string(init_res.error())).c_str(), IQ_DIR);
        return EXIT_FAILURE;
    }
    std::printf("ISP 初始化成功\n\n");

    // ---------------------------------------------------------------
    // 2. 读取并打印当前曝光信息
    // ---------------------------------------------------------------
    std::printf("--- 当前曝光参数 ---\n");
    auto exp_info = isp.get_exposure_info();
    if (exp_info.ok()) {
        std::printf("  曝光时间 : %.3f ms\n", (*exp_info).time_ms);
        std::printf("  ISO      : %d\n",      (*exp_info).iso);
        std::printf("  模拟增益 : %.2f\n",    (*exp_info).analog_gain);
        std::printf("  数字增益 : %.2f\n",    (*exp_info).digital_gain);
    } else {
        std::printf("  （无法获取当前曝光信息）\n");
    }
    std::printf("\n");

    // ---------------------------------------------------------------
    // 3. 设置手动曝光：10ms，ISO 400
    // ---------------------------------------------------------------
    std::printf("--- 设置手动曝光 (10ms, ISO 400) ---\n");
    {
        rv1126b::ISP::ExposureConfig exp_cfg;
        exp_cfg.mode    = rv1126b::ISP::ExposureMode::Manual;
        exp_cfg.time_ms = 10.0f;
        exp_cfg.iso     = 400;

        auto res = isp.set_exposure(exp_cfg);
        if (res.ok())
            std::printf("  手动曝光已设置\n");
        else
            std::fprintf(stderr, "  [警告] 设置曝光失败: %s\n",
                         std::string(rv1126b::to_string(res.error())).c_str());
    }

    // ---------------------------------------------------------------
    // 4. 设置白平衡：5500K（日光色温）
    // ---------------------------------------------------------------
    std::printf("--- 设置白平衡 (5500K) ---\n");
    {
        rv1126b::ISP::WhiteBalanceConfig wb_cfg;
        wb_cfg.mode       = rv1126b::ISP::WBMode::ColorTemp;
        wb_cfg.color_temp = 5500;

        auto res = isp.set_white_balance(wb_cfg);
        if (res.ok())
            std::printf("  白平衡已设置为 5500K\n");
        else
            std::fprintf(stderr, "  [警告] 设置白平衡失败: %s\n",
                         std::string(rv1126b::to_string(res.error())).c_str());
    }

    // ---------------------------------------------------------------
    // 5. 设置色彩参数：饱和度 / 锐度 / 对比度
    // ---------------------------------------------------------------
    std::printf("--- 设置色彩参数 (饱和度 60, 锐度 55, 对比度 58) ---\n");
    {
        rv1126b::ISP::ColorConfig color_cfg;
        color_cfg.saturation = 60;   // 0-100，50=默认
        color_cfg.sharpness  = 55;
        color_cfg.contrast   = 58;

        auto res = isp.set_color(color_cfg);
        if (res.ok())
            std::printf("  色彩参数已设置\n");
        else
            std::fprintf(stderr, "  [警告] 设置色彩失败: %s\n",
                         std::string(rv1126b::to_string(res.error())).c_str());
    }

    // ---------------------------------------------------------------
    // 6. 设置降噪
    // ---------------------------------------------------------------
    std::printf("--- 设置降噪（空域+时域，强度 70）---\n");
    {
        rv1126b::ISP::NoiseReductionConfig nr_cfg;
        nr_cfg.enable_spatial  = true;
        nr_cfg.enable_temporal = true;
        nr_cfg.strength        = 70;   // 0-100

        auto res = isp.set_noise_reduction(nr_cfg);
        if (res.ok())
            std::printf("  降噪参数已设置\n");
        else
            std::fprintf(stderr, "  [警告] 设置降噪失败: %s\n",
                         std::string(rv1126b::to_string(res.error())).c_str());
    }

    // ---------------------------------------------------------------
    // 7. 启用 2x HDR 模式
    // ---------------------------------------------------------------
    std::printf("--- 启用 2x HDR 模式 ---\n");
    {
        auto res = isp.set_hdr_mode(rv1126b::ISP::HDRMode::HDR2x);
        if (res.ok())
            std::printf("  HDR 2x 已启用\n");
        else
            std::fprintf(stderr, "  [警告] 设置 HDR 失败: %s\n",
                         std::string(rv1126b::to_string(res.error())).c_str());
    }

    // ---------------------------------------------------------------
    // 8. 等待 3 秒，让 ISP 参数生效并稳定
    // ---------------------------------------------------------------
    std::printf("\n等待 3 秒观察效果...\n");
    for (int i = 3; i > 0 && !g_stop.load(); --i) {
        std::printf("  %d...\n", i);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // ---------------------------------------------------------------
    // 9. 恢复全自动模式
    // ---------------------------------------------------------------
    if (!g_stop.load()) {
        std::printf("\n--- 恢复自动模式 ---\n");

        // 自动曝光
        rv1126b::ISP::ExposureConfig auto_exp;
        auto_exp.mode = rv1126b::ISP::ExposureMode::Auto;
        isp.set_exposure(auto_exp);

        // 自动白平衡
        rv1126b::ISP::WhiteBalanceConfig auto_wb;
        auto_wb.mode = rv1126b::ISP::WBMode::Auto;
        isp.set_white_balance(auto_wb);

        // 关闭 HDR
        isp.set_hdr_mode(rv1126b::ISP::HDRMode::Off);

        std::printf("  已恢复自动曝光、自动白平衡，HDR 已关闭\n");
    }

    std::printf("\n程序退出。\n");
    return EXIT_SUCCESS;
}
