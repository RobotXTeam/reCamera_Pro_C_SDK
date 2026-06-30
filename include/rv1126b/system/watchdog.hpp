/**
 * @file watchdog.hpp
 * @brief RV1126B SDK — 硬件看门狗模块
 *
 * 基于 Linux /dev/watchdog 接口（WDIOF_SETTIMEOUT）。
 * 若 feed() 未在超时时间内调用，硬件将重启系统。
 *
 * 注意：start() 后必须持续调用 feed()，否则系统将重启。
 * stop() 向 /dev/watchdog 写入 'V' magic close 字符后关闭。
 *
 * @code
 * Watchdog wd;
 * wd.start(10);  // 10 秒超时
 *
 * // 主循环中
 * while (running) {
 *     wd.feed();
 *     do_work();
 * }
 * wd.stop();
 * @endcode
 */
#pragma once

#include <memory>

#include "rv1126b/core/error.hpp"
#include <cstdint>

namespace rv1126b {

class Watchdog {
public:
    Watchdog();
    ~Watchdog();

    Watchdog(const Watchdog&)            = delete;
    Watchdog& operator=(const Watchdog&) = delete;

    /**
     * @brief 打开并启动看门狗
     * @param timeout_sec  超时时间（秒），实际超时由内核驱动决定（可能向上取整）
     * @param device       看门狗设备路径，默认 /dev/watchdog
     */
    Result<void> start(int timeout_sec = 10,
                       const char* device = "/dev/watchdog");

    /**
     * @brief 喂狗（复位超时计时器）
     * 向 /dev/watchdog 写入任意字节（或使用 WDIOC_KEEPALIVE ioctl）
     */
    Result<void> feed();

    /**
     * @brief 停止看门狗
     * 向设备写入 'V'（magic close），然后关闭 fd。
     * 仅当驱动支持 WDIOF_MAGICCLOSE 时才真正停止；
     * 否则设备关闭后会立即触发复位。
     */
    Result<void> stop();

    bool is_running() const noexcept;

    /// 获取当前设置的超时时间（秒），由驱动实际应用的值
    Result<int> get_timeout() const;

    /// 获取看门狗状态（WDIOF_* 标志位）
    Result<uint32_t> get_status() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
