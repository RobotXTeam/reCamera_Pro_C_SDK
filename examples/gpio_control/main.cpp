// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// gpio_control — GPIO 控制示例
// 演示 GPIO 输出（LED 闪烁）和 GPIO 输入（电平检测轮询）。
//
// GPIO 编号计算规则：
//   全局引脚号 = 控制器编号 × 32 + 引脚组偏移
//   例：GPIO2_PB6 → 控制器2 × 32 + 引脚组B偏移6 = 64 + 8 + 6 = 78
//       GPIO0_PA0 → 控制器0 × 32 + 0 = 0
//
// 要确认板上可用的 GPIO，请在设备上执行：
//   cat /sys/kernel/debug/gpio
//   ls /sys/class/gpio/

#include <rv1126b/gpio.hpp>
#include <rv1126b/logger.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

// ---------------------------------------------------------------
// 信号标志：Ctrl+C 触发后设为 true，主循环退出
// ---------------------------------------------------------------
static std::atomic<bool> g_stop{false};

static void signal_handler(int /*sig*/)
{
    g_stop.store(true, std::memory_order_relaxed);
}

int main()
{
    // 注册信号处理
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Info);

    // ---------------------------------------------------------------
    // 1. GPIO 输出示例：LED 闪烁
    //    优先使用 GPIO2_PB6 (pin 78)；如果板子上该引脚不可用，
    //    可以改为 GPIO0_PA0 (pin 0) 或其他空闲引脚。
    // ---------------------------------------------------------------
    // 请根据您的具体硬件连线，修改对应的 GPIO 编号
    constexpr int LED_PIN   = 0;    // TODO: 填写输出引脚编号
    constexpr int INPUT_PIN = 1;    // TODO: 填写输入引脚编号

    std::printf("=== GPIO 输出示例：LED 闪烁 (pin %d) ===\n", LED_PIN);

    rv1126b::GPIO led(LED_PIN);

    {
        auto res = led.open(rv1126b::GPIO::Direction::Output);
        if (!res.ok()) {
            std::fprintf(stderr, "[警告] 打开 GPIO pin %d 失败: %s\n"
                         "       请检查引脚编号或尝试 GPIO0_PA0 (pin 0)\n",
                         LED_PIN,
                         std::string(rv1126b::to_string(res.error())).c_str());
            // 回退到 pin 0
        } else {
            std::printf("GPIO pin %d 已配置为输出模式\n", LED_PIN);
            std::printf("开始闪烁，按 Ctrl+C 停止...\n\n");

            int blink_count = 0;
            while (!g_stop.load(std::memory_order_relaxed) && blink_count < 10) {
                // 点亮
                led.set(1);
                std::printf("  [%2d] LED ON\n", ++blink_count);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // 熄灭
                led.set(0);
                std::printf("  [%2d] LED OFF\n", blink_count);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            led.close();
            std::printf("GPIO 输出示例完成。\n\n");
        }
    }

    // ---------------------------------------------------------------
    // 2. GPIO 输入示例：电平检测轮询
    //    每 100ms 采样一次，连续读取 5 秒
    // ---------------------------------------------------------------
    if (g_stop.load()) goto cleanup;

    std::printf("=== GPIO 输入示例：电平检测 (pin %d) ===\n", INPUT_PIN);

    {
        rv1126b::GPIO input_gpio(INPUT_PIN);
        auto res = input_gpio.open(rv1126b::GPIO::Direction::Input);
        if (!res.ok()) {
            std::fprintf(stderr, "[警告] 打开输入 GPIO pin %d 失败: %s\n",
                         INPUT_PIN,
                         std::string(rv1126b::to_string(res.error())).c_str());
        } else {
            std::printf("GPIO pin %d 已配置为输入模式，读取 5 秒...\n", INPUT_PIN);

            int last_level = -1;
            auto start = std::chrono::steady_clock::now();

            while (!g_stop.load(std::memory_order_relaxed)) {
                auto elapsed = std::chrono::steady_clock::now() - start;
                if (elapsed >= std::chrono::seconds(5)) break;

                auto level_res = input_gpio.get();
                if (!level_res.ok()) {
                    std::fprintf(stderr, "[错误] 读取 GPIO 失败: %s\n",
                                 std::string(rv1126b::to_string(level_res.error())).c_str());
                    break;
                }

                int level = *level_res;
                if (level != last_level) {
                    double secs = std::chrono::duration<double>(elapsed).count();
                    std::printf("  [%.2fs] GPIO pin %d 电平变化 -> %s\n",
                                secs, INPUT_PIN, level ? "HIGH" : "LOW");
                    last_level = level;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            input_gpio.close();
            std::printf("GPIO 输入示例完成。\n");
        }
    }

cleanup:
    std::printf("\n程序退出。\n");
    return EXIT_SUCCESS;
}
