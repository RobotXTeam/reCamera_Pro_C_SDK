/**
 * @file pwm.hpp
 * @brief RV1126B SDK — PWM module
 *
 * Based on Linux sysfs /sys/class/pwm/ interface.
 * RV1126B has multiple PWM channels (PWM0~PWM11) registered as pwmchip via DTS.
 *
 * @code
 * PWM pwm;
 * pwm.open(0, 0);              // pwmchip0, channel 0
 * pwm.set_period_ns(1000000);  // 1 kHz
 * pwm.set_duty_ns(500000);     // 50% duty cycle
 * pwm.set_enable(true);
 * // ...
 * pwm.set_enable(false);
 * pwm.close();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <cstdint>
#include <memory>

namespace rv1126b {

class PWM {
public:
    enum class Polarity {
        Normal   = 0,   ///< Active high
        Inversed = 1,   ///< Active low (inverted)
    };

    PWM();
    ~PWM();

    PWM(const PWM&)            = delete;
    PWM& operator=(const PWM&) = delete;
    PWM(PWM&&) noexcept;
    PWM& operator=(PWM&&) noexcept;

    /**
     * @brief Open a PWM channel
     * @param chip_num    pwmchip number (/sys/class/pwm/pwmchipN)
     * @param channel     Channel number (pwmchipN/pwmM)
     */
    Result<void> open(int chip_num, int channel);

    /// Close and release the channel (unexport)
    void close();

    bool is_open() const noexcept;

    /**
     * @brief Set PWM period
     * @param period_ns Period (nanoseconds), e.g. 1000000 = 1 kHz
     */
    Result<void> set_period_ns(uint64_t period_ns);

    /**
     * @brief Set duty cycle
     * @param duty_ns High-level duration (nanoseconds), must be <= period_ns
     */
    Result<void> set_duty_ns(uint64_t duty_ns);

    /**
     * @brief Set duty cycle (percentage)
     * @param percent 0.0 ~ 100.0
     */
    Result<void> set_duty_percent(float percent);

    /**
     * @brief Set frequency (Hz), duty cycle ratio is preserved
     */
    Result<void> set_frequency(uint32_t freq_hz);

    /// Set polarity
    Result<void> set_polarity(Polarity polarity);

    /// Enable/disable output
    Result<void> set_enable(bool enable);

    bool is_enabled() const noexcept;

    uint64_t period_ns() const noexcept;
    uint64_t duty_ns() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
