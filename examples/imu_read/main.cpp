// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// imu_read — IMU 数据读取示例
// 自动检测 IIO 设备（/dev/iio:device0 或 iio:device1），
// 以约 50Hz 的频率采样 100 次，打印加速度计/陀螺仪/温度数据，
// 最终统计并打印 min/max/avg。

#include <rv1126b/imu.hpp>
#include <rv1126b/logger.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <thread>

static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

// 统计辅助结构
struct Stats {
    double min_val =  std::numeric_limits<double>::max();
    double max_val = -std::numeric_limits<double>::max();
    double sum     = 0.0;
    int    count   = 0;

    void update(double v) {
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
        sum   += v;
        count += 1;
    }

    double avg() const { return count > 0 ? sum / count : 0.0; }
};

int main()
{
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    std::printf("=== IMU 数据读取示例 ===\n\n");

    // ---------------------------------------------------------------
    // 1. 初始化 IMU（自动检测 /dev/iio:device0 或 iio:device1）
    // ---------------------------------------------------------------
    rv1126b::IMU imu;
    auto init_res = imu.init();
    if (!init_res.ok()) {
        std::fprintf(stderr, "[错误] IMU 初始化失败: %s\n",
                     std::string(rv1126b::to_string(init_res.error())).c_str());
        std::fprintf(stderr, "       请确认 IIO 设备存在: ls /sys/bus/iio/devices/\n");
        return EXIT_FAILURE;
    }

    std::printf("IMU 初始化成功，开始以 50Hz 采样 100 次...\n");
    std::printf("%-6s  %8s %8s %8s  %8s %8s %8s  %7s\n",
                "采样#", "accel_x", "accel_y", "accel_z",
                "gyro_x",  "gyro_y",  "gyro_z",  "温度(°C)");
    std::printf("%s\n", std::string(72, '-').c_str());

    // ---------------------------------------------------------------
    // 2. 统计变量
    // ---------------------------------------------------------------
    Stats ax, ay, az, gx, gy, gz, temp;
    int sample = 0;
    constexpr int   TARGET_SAMPLES   = 100;
    constexpr auto  SAMPLE_INTERVAL  = std::chrono::milliseconds(20); // 50 Hz

    // ---------------------------------------------------------------
    // 3. 采样循环
    // ---------------------------------------------------------------
    while (sample < TARGET_SAMPLES && !g_stop.load(std::memory_order_relaxed)) {
        auto t0 = std::chrono::steady_clock::now();

        auto res = imu.read();
        if (!res.ok()) {
            std::fprintf(stderr, "[错误] IMU 读取失败: %s\n",
                         std::string(rv1126b::to_string(res.error())).c_str());
            break;
        }

        const auto& d = *res;
        ax.update(d.accel_x);  ay.update(d.accel_y);  az.update(d.accel_z);
        gx.update(d.gyro_x);   gy.update(d.gyro_y);   gz.update(d.gyro_z);
        temp.update(d.temperature);

        ++sample;
        std::printf("[%4d]  %8.3f %8.3f %8.3f  %8.3f %8.3f %8.3f  %7.2f\n",
                    sample,
                    d.accel_x, d.accel_y, d.accel_z,
                    d.gyro_x,  d.gyro_y,  d.gyro_z,
                    d.temperature);

        // 精确定时：睡眠剩余时间以维持 50Hz
        auto elapsed = std::chrono::steady_clock::now() - t0;
        if (elapsed < SAMPLE_INTERVAL)
            std::this_thread::sleep_for(SAMPLE_INTERVAL - elapsed);
    }

    // ---------------------------------------------------------------
    // 4. 打印统计摘要
    // ---------------------------------------------------------------
    std::printf("\n=== 统计摘要（共 %d 次采样） ===\n", sample);
    std::printf("%-12s  %10s %10s %10s\n", "通道", "最小值", "最大值", "平均值");
    std::printf("%s\n", std::string(46, '-').c_str());

    auto print_stat = [](const char* name, const Stats& s) {
        std::printf("%-12s  %10.4f %10.4f %10.4f\n",
                    name, s.min_val, s.max_val, s.avg());
    };

    print_stat("accel_x (g)",  ax);
    print_stat("accel_y (g)",  ay);
    print_stat("accel_z (g)",  az);
    print_stat("gyro_x (°/s)", gx);
    print_stat("gyro_y (°/s)", gy);
    print_stat("gyro_z (°/s)", gz);
    print_stat("温度 (°C)",     temp);

    std::printf("\n程序退出。\n");
    return EXIT_SUCCESS;
}
