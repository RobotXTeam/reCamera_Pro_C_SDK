/**
 * @file adc.hpp
 * @brief RV1126B SDK — ADC module
 *
 * Reads ADC values via the Linux IIO (Industrial I/O) sysfs interface.
 * RV1126B has a built-in SARADC exposed through the IIO framework at
 * /sys/bus/iio/devices/iio:deviceN/
 *
 * @code
 * ADC adc;
 * adc.open("rk-saradc");   // find IIO device by name
 * auto val = adc.read_channel(0);
 * if (val) {
 *     float voltage = *val * adc.scale() + adc.offset();
 * }
 * adc.close();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace rv1126b {

class ADC {
public:
    ADC();
    ~ADC();

    ADC(const ADC&)            = delete;
    ADC& operator=(const ADC&) = delete;
    ADC(ADC&&) noexcept;
    ADC& operator=(ADC&&) noexcept;

    /**
     * @brief Open the IIO ADC device
     * @param device_name IIO device name (from /sys/bus/iio/devices/iio:deviceN/name)
     *                    Pass empty string to open the first available IIO device
     * @return Ok or error code
     */
    Result<void> open(const std::string& device_name = "rk-saradc");

    /// Close the device
    void close();

    bool is_open() const noexcept;

    /**
     * @brief Read raw ADC value (integer count)
     * @param channel Channel number (0-based)
     * @return Raw ADC count
     */
    Result<int32_t> read_channel(int channel);

    /**
     * @brief Read voltage (mV)
     *        Voltage = (raw + offset) * scale (in mV)
     * @param channel Channel number
     */
    Result<float> read_voltage_mv(int channel);

    /**
     * @brief Get channel scale (mV/count)
     */
    Result<float> get_scale(int channel);

    /**
     * @brief Get channel offset (count)
     */
    Result<int32_t> get_offset(int channel);

    /// IIO device path, e.g. /sys/bus/iio/devices/iio:device0
    const std::string& device_path() const noexcept;

    /// Number of detected channels
    int channel_count() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
