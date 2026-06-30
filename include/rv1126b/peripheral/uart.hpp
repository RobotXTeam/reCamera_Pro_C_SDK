/**
 * @file uart.hpp
 * @brief RV1126B SDK — UART (serial port) module
 *
 * @code
 * UART uart("/dev/ttyS2");
 * uart.open(UART::Config{.baud=115200, .data_bits=8, .parity=UART::Parity::None});
 * uart.write("hello\n", 6);
 * char buf[64];
 * int n = uart.read(buf, sizeof(buf), 100);
 * uart.close();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <string>
#include <cstdint>

namespace rv1126b {

class UART {
public:
    enum class Parity    { None = 0, Odd, Even };
    enum class StopBits  { One = 1, Two = 2 };
    enum class FlowCtrl  { None = 0, Hardware, Software };

    struct Config {
        int      baud       {115200};
        int      data_bits  {8};        ///< 5/6/7/8
        Parity   parity     {Parity::None};
        StopBits stop_bits  {StopBits::One};
        FlowCtrl flow_ctrl  {FlowCtrl::None};
    };

    explicit UART(std::string device = "/dev/ttyS2");
    ~UART();

    UART(const UART&)            = delete;
    UART& operator=(const UART&) = delete;

    Result<void> open();
    Result<void> open(const Config& config);
    void close();
    bool is_open() const noexcept;

    /// Send data; returns actual bytes sent
    Result<size_t> write(const void* data, size_t len);

    /**
     * @brief Read data
     * @param buf       Receive buffer
     * @param len       Maximum bytes to read
     * @param timeout_ms Timeout in ms; 0=non-blocking, -1=block indefinitely
     * @return Actual bytes read
     */
    Result<size_t> read(void* buf, size_t len, int timeout_ms = 100);

    /// Flush transmit and receive buffers
    Result<void> flush();

    /// Get current configuration
    const Config& config() const noexcept { return config_; }
    const std::string& device() const noexcept { return device_; }

private:
    std::string device_;
    Config      config_{};
    int         fd_{-1};
};

} // namespace rv1126b
