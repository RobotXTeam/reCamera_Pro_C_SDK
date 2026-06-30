// Copyright (c) 2024 RV1126B SDK Project
// SPDX-License-Identifier: MIT
//
// hello_world — SDK 入门示例
// 演示如何初始化日志、获取系统信息并打印 SDK 版本。

#include <rv1126b/logger.hpp>
#include <rv1126b/sysinfo.hpp>
#include <rv1126b/version.hpp>

#include <cstdio>
#include <cstdlib>

int main()
{
    // ---------------------------------------------------------------
    // 1. 配置日志：将级别设置为 Debug，方便调试时查看详细输出
    // ---------------------------------------------------------------
    auto& log = rv1126b::Logger::get();
    log.set_level(rv1126b::LogLevel::Debug);
    // 也可以将日志同步写入文件（可选）
    // log.enable_file("/tmp/rv1126b.log");

    // ---------------------------------------------------------------
    // 2. 打印 SDK 版本号
    // ---------------------------------------------------------------
    std::printf("========================================\n");
    std::printf("  RV1126B SDK  v%s\n", RV1126B_SDK_VERSION_STRING);
    std::printf("========================================\n\n");

    // ---------------------------------------------------------------
    // 3. 获取系统信息
    // ---------------------------------------------------------------
    auto result = rv1126b::sys::get_info();
    if (!result.ok()) {
        std::fprintf(stderr, "[错误] 获取系统信息失败: %s\n",
                     std::string(rv1126b::to_string(result.error())).c_str());
        return EXIT_FAILURE;
    }

    const auto& info = *result;
    std::printf("设备名称  : %s\n", info.device_name.c_str());
    std::printf("芯片型号  : %s\n", info.chip_name.c_str());
    std::printf("内核版本  : %s\n", info.kernel_version.c_str());
    std::printf("NPU 核心数: %d\n", info.npu_cores);
    std::printf("CPU 温度  : %.1f °C\n", info.cpu_temp);
    std::printf("\n");

    // ---------------------------------------------------------------
    // 4. 打印问候语
    // ---------------------------------------------------------------
    std::printf("Hello, RV1126B SDK!\n");

    return EXIT_SUCCESS;
}
