// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// uart_echo — UART 回显示例
// 打开 /dev/ttyS2，将收到的每个字节原样回送给发送方。
// 支持超时读取（非阻塞模式），按 Ctrl+C 优雅退出。

#include <rv1126b/logger.hpp>
#include <rv1126b/uart.hpp>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

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

    // ---------------------------------------------------------------
    // 1. 打开串口
    //    设备路径：/dev/ttyS2（RV1126B 板上的调试串口）
    //    备用路径：/dev/ttyS4
    // ---------------------------------------------------------------
    // 请根据您的具体硬件连线，修改对应的 UART 节点
    constexpr const char* UART_DEV = "/dev/ttyS0"; // TODO: 填写串口设备节点

    std::printf("=== UART 回显示例 ===\n");
    std::printf("设备: %s  波特率: 115200\n\n", UART_DEV);

    rv1126b::UART uart(UART_DEV);

    rv1126b::UART::Config cfg;
    cfg.baud      = 115200;
    cfg.data_bits = 8;
    cfg.stop_bits = rv1126b::UART::StopBits::One;
    cfg.parity    = rv1126b::UART::Parity::None;

    auto open_res = uart.open(cfg);
    if (!open_res.ok()) {
        std::fprintf(stderr, "[错误] 打开 %s 失败: %s\n",
                     UART_DEV,
                     std::string(rv1126b::to_string(open_res.error())).c_str());
        return EXIT_FAILURE;
    }

    std::printf("串口已打开，进入回显循环...\n");
    std::printf("在另一终端执行: echo 'hello' > /dev/ttyS2\n");
    std::printf("按 Ctrl+C 退出。\n\n");

    // ---------------------------------------------------------------
    // 2. 回显循环
    // ---------------------------------------------------------------
    std::vector<uint8_t> rx_buf(256);
    uint64_t total_rx = 0;
    uint64_t total_tx = 0;

    while (!g_stop.load(std::memory_order_relaxed)) {
        // 带超时的读操作（100ms 内无数据则返回 0 字节）
        auto read_res = uart.read(rx_buf.data(), rx_buf.size());
        if (!read_res.ok()) {
            std::fprintf(stderr, "[错误] UART 读取失败: %s\n",
                         std::string(rv1126b::to_string(read_res.error())).c_str());
            break;
        }

        size_t n = *read_res;
        if (n == 0) {
            // 超时，继续等待
            continue;
        }

        total_rx += n;
        std::printf("[RX %4zu 字节]", n);
        for (size_t i = 0; i < n && i < 16; ++i)
            std::printf(" %02X", rx_buf[i]);
        if (n > 16) std::printf(" ...");
        std::printf("\n");

        // 将接收到的数据原样回送
        auto write_res = uart.write(rx_buf.data(), n);
        if (!write_res.ok()) {
            std::fprintf(stderr, "[错误] UART 写入失败: %s\n",
                         std::string(rv1126b::to_string(write_res.error())).c_str());
            break;
        }

        total_tx += *write_res;
        std::printf("[TX %4zu 字节] 回显完成\n", *write_res);
    }

    // ---------------------------------------------------------------
    // 3. 统计并关闭
    // ---------------------------------------------------------------
    uart.close();

    std::printf("\n=== 统计 ===\n");
    std::printf("总接收: %llu 字节\n", (unsigned long long)total_rx);
    std::printf("总发送: %llu 字节\n", (unsigned long long)total_tx);
    std::printf("程序退出。\n");

    return EXIT_SUCCESS;
}
