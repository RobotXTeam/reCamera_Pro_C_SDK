/**
 * @file sysinfo.hpp
 * @brief RV1126B SDK — system information module
 *
 * Reads /proc/cpuinfo, /proc/meminfo, /sys/class/thermal/, etc.
 * to obtain CPU temperature, memory, chip model, and other system information.
 *
 * @code
 * auto info = SysInfo::get_info();
 * if (info) {
 *     printf("chip: %s, %d NPU cores\n",
 *            info->chip_name.c_str(), info->npu_cores);
 *     printf("CPU temp: %.1f°C\n", info->cpu_temp);
 *     printf("Memory: %d / %d MB free\n",
 *            info->memory_free_mb, info->memory_total_mb);
 * }
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <string>
#include <vector>

namespace rv1126b {

struct SysInfo {
    std::string device_name;        ///< hostname (/proc/sys/kernel/hostname)
    std::string kernel_version;     ///< kernel version string (uname)
    std::string chip_name;          ///< Hardware model from /proc/cpuinfo
    std::string cpu_model;          ///< CPU model string
    int         cpu_cores       {0};
    int         npu_cores       {1};    ///< RV1126B has 1 NPU core (1 TOPS)
    float       cpu_temp        {0.0f}; ///< CPU temperature (°C)
    int         memory_total_mb {0};
    int         memory_free_mb  {0};
    int         memory_used_mb  {0};
    int         swap_total_mb   {0};
    int         swap_free_mb    {0};
    uint64_t    uptime_sec      {0};    ///< System uptime (seconds)
};

struct CpuStat {
    float    usage_percent  {0.0f};     ///< Overall CPU usage (%)
    std::vector<float> per_core;        ///< Per-core usage (%)
};

namespace sys {

/**
 * @brief Get complete system information (one-shot read)
 */
Result<SysInfo> get_info();

/**
 * @brief Get CPU temperature (°C)
 * Reads /sys/class/thermal/thermal_zone0/temp
 */
Result<float> get_cpu_temp();

/**
 * @brief Get memory information (MB)
 * @param total_mb  Output: total memory
 * @param free_mb   Output: available memory
 */
Result<void> get_memory_info(int& total_mb, int& free_mb);

/**
 * @brief Calculate CPU usage (requires two consecutive sampling intervals)
 * Note: first call returns 0; call again after at least 100ms.
 */
Result<CpuStat> get_cpu_usage();

/**
 * @brief Get system uptime (seconds)
 */
Result<uint64_t> get_uptime_sec();

/**
 * @brief Get kernel version string
 */
Result<std::string> get_kernel_version();

/**
 * @brief Get hostname
 */
Result<std::string> get_hostname();

} // namespace sys
} // namespace rv1126b
