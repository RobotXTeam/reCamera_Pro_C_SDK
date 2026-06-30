/**
 * @file imu.hpp
 * @brief RV1126B SDK — IMU module (ICM-42670)
 *
 * Reads ICM-42670 6-axis IMU data via the Linux IIO (Industrial I/O) sysfs interface.
 * ICM-42670 connects to RV1126B over I2C/SPI; the Linux kernel exposes data through the iio driver.
 *
 * @code
 * IMU imu;
 * imu.init();  // auto-detect icm42670
 *
 * auto data = imu.read();
 * if (data) {
 *     printf("accel: %.3f %.3f %.3f m/s²\n",
 *            data->accel_x, data->accel_y, data->accel_z);
 *     printf("gyro:  %.3f %.3f %.3f deg/s\n",
 *            data->gyro_x, data->gyro_y, data->gyro_z);
 * }
 * imu.deinit();
 * @endcode
 */
#pragma once

#include <memory>

#include "rv1126b/core/error.hpp"
#include <cstdint>
#include <string>

namespace rv1126b {

struct ImuData {
    float   accel_x;        ///< Acceleration X axis (m/s²)
    float   accel_y;        ///< Acceleration Y axis (m/s²)
    float   accel_z;        ///< Acceleration Z axis (m/s²)
    float   gyro_x;         ///< Angular velocity X axis (deg/s)
    float   gyro_y;         ///< Angular velocity Y axis (deg/s)
    float   gyro_z;         ///< Angular velocity Z axis (deg/s)
    float   temperature;    ///< Temperature (°C)
    int64_t timestamp_ns;   ///< Timestamp (nanoseconds, monotonic clock)
};

class IMU {
public:
    enum class AccelRange {
        G2  = 0,    ///< ±2 g
        G4  = 1,    ///< ±4 g
        G8  = 2,    ///< ±8 g
        G16 = 3,    ///< ±16 g
    };

    enum class GyroRange {
        DPS250  = 0,    ///< ±250 deg/s
        DPS500  = 1,    ///< ±500 deg/s
        DPS1000 = 2,    ///< ±1000 deg/s
        DPS2000 = 3,    ///< ±2000 deg/s
    };

    struct Config {
        std::string device_name  {"icm42670"};  ///< IIO device name; empty string = auto-search
        AccelRange  accel_range  {AccelRange::G8};
        GyroRange   gyro_range   {GyroRange::DPS2000};
        int         sample_rate  {100};          ///< Sample rate (Hz); 0 = use driver default
    };

    IMU();
    ~IMU();

    IMU(const IMU&)            = delete;
    IMU& operator=(const IMU&) = delete;
    IMU(IMU&&) noexcept;
    IMU& operator=(IMU&&) noexcept;

    /**
     * @brief Initialize the IMU
     * @param config  Configuration; auto-searches IIO device when device_name is empty
     */
    Result<void> init();
    Result<void> init(const Config& config);

    /// Release resources
    void deinit();

    bool is_initialized() const noexcept;

    /**
     * @brief Read one IMU sample
     * Reads sysfs in_accel_x/y/z_raw, in_anglvel_x/y/z_raw, in_temp_raw
     * and multiplies by the corresponding scale to convert to physical units
     */
    Result<ImuData> read();

    /**
     * @brief Read raw accelerometer value (integer count)
     */
    Result<int32_t> read_accel_raw(int axis);  ///< axis: 0=X, 1=Y, 2=Z

    /**
     * @brief Read raw gyroscope value
     */
    Result<int32_t> read_gyro_raw(int axis);

    /**
     * @brief Read raw temperature value
     */
    Result<int32_t> read_temp_raw();

    /// IIO device path, e.g. /sys/bus/iio/devices/iio:device1
    const std::string& iio_device_path() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
