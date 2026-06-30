/**
 * @file gpio.hpp
 * @brief RV1126B SDK — GPIO module
 *
 * Based on Linux sysfs interface, supports RV1126B GPIO0~4 and GPIO6 (5+1 banks).
 *
 * @code
 * // Output control
 * GPIO gpio(GPIO::bank(2, GPIO::PB6));  // GPIO2_PB6 = global pin 54
 * gpio.open(GPIO::Direction::Output);
 * gpio.set(1);
 * gpio.close();
 *
 * // Input read
 * GPIO btn(GPIO::bank(0, GPIO::PA5));
 * btn.open(GPIO::Direction::Input);
 * int v = btn.get();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <cstdint>

namespace rv1126b {

class GPIO {
public:
    // ─── Pin bank constants (aligned with Linux sysfs) ──────────────────────────

    // Pin offsets within a bank
    static constexpr int PA0=0,  PA1=1,  PA2=2,  PA3=3;
    static constexpr int PA4=4,  PA5=5,  PA6=6,  PA7=7;
    static constexpr int PB0=8,  PB1=9,  PB2=10, PB3=11;
    static constexpr int PB4=12, PB5=13, PB6=14, PB7=15;
    static constexpr int PC0=16, PC1=17, PC2=18, PC3=19;
    static constexpr int PC4=20, PC5=21, PC6=22, PC7=23;
    static constexpr int PD0=24, PD1=25, PD2=26, PD3=27;
    static constexpr int PD4=28, PD5=29, PD6=30, PD7=31;

    /// Calculate global GPIO pin number (used by sysfs)
    static constexpr uint32_t bank(int gpio_bank, int pin_offset) {
        return static_cast<uint32_t>(gpio_bank * 32 + pin_offset);
    }

    // ─── Direction ────────────────────────────────────────────────────────────

    enum class Direction {
        Input  = 0,
        Output = 1,
    };

    // ─── Edge trigger ────────────────────────────────────────────────────────

    enum class EdgeTrigger {
        None    = 0,
        Rising  = 1,
        Falling = 2,
        Both    = 3,
    };

    // ─── Constructor/destructor ──────────────────────────────────────────────

    explicit GPIO(uint32_t global_pin);
    ~GPIO();

    GPIO(const GPIO&)            = delete;
    GPIO& operator=(const GPIO&) = delete;
    GPIO(GPIO&&) noexcept;
    GPIO& operator=(GPIO&&) noexcept;

    // ─── Control ─────────────────────────────────────────────────────────────

    /// Export the GPIO and set its direction
    Result<void> open(Direction dir);

    /// Unexport the GPIO and release resources
    void close();

    bool is_open() const noexcept;

    /// Output high/low level (valid in Output mode only)
    Result<void> set(int value);

    /// Read current level
    Result<int> get() const;

    /// Set edge trigger mode (Input mode only)
    Result<void> set_edge(EdgeTrigger edge);

    /**
     * @brief Block waiting for a GPIO interrupt (only valid after set_edge())
     * @param timeout_ms Timeout in milliseconds; -1 = block indefinitely
     * @return Level value (0/1) when triggered, Error::Timeout on timeout
     */
    Result<int> poll(int timeout_ms = -1);

    uint32_t pin() const noexcept { return pin_; }

private:
    uint32_t pin_;
    int      value_fd_{-1};
    bool     exported_{false};
    Direction dir_{Direction::Input};
};

} // namespace rv1126b
