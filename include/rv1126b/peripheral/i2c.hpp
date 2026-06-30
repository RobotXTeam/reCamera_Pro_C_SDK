/**
 * @file i2c.hpp
 * @brief RV1126B SDK — I2C module
 *
 * Based on Linux /dev/i2c-N interface; supports SMBus and raw I2C transfers.
 *
 * @code
 * I2C i2c;
 * i2c.open(1);  // /dev/i2c-1
 * uint8_t val;
 * i2c.read_reg(0x68, 0x75, &val, 1);  // read MPU6050 WHO_AM_I
 * i2c.close();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace rv1126b {

class I2C {
public:
    /// I2C transfer descriptor (used by transfer() for combined read/write)
    struct Message {
        uint16_t addr;      ///< 7-bit device address
        bool     read;      ///< true=read, false=write
        uint8_t* buf;       ///< Data buffer
        uint16_t len;       ///< Data length
        uint16_t flags;     ///< Additional flags (0 is usually fine)
    };

    I2C();
    ~I2C();

    I2C(const I2C&)            = delete;
    I2C& operator=(const I2C&) = delete;
    I2C(I2C&&) noexcept;
    I2C& operator=(I2C&&) noexcept;

    /**
     * @brief Open the I2C bus
     * @param bus_num Bus number, e.g. 1 corresponds to /dev/i2c-1
     */
    Result<void> open(int bus_num);

    /// Close the bus
    void close();

    bool is_open() const noexcept;

    /**
     * @brief Write register (8-bit register address, single byte)
     * @param dev_addr  7-bit device address
     * @param reg       Register address
     * @param data      Value to write
     */
    Result<void> write_reg8(uint8_t dev_addr, uint8_t reg, uint8_t data);

    /**
     * @brief Write register (8-bit register address, multiple bytes)
     */
    Result<void> write_reg(uint8_t dev_addr, uint8_t reg,
                           const uint8_t* data, size_t len);

    /**
     * @brief Write register (16-bit register address)
     */
    Result<void> write_reg16(uint8_t dev_addr, uint16_t reg,
                             const uint8_t* data, size_t len);

    /**
     * @brief Read register (8-bit register address)
     * @param dev_addr  7-bit device address
     * @param reg       Register address
     * @param buf       Receive buffer
     * @param len       Bytes to read
     */
    Result<void> read_reg(uint8_t dev_addr, uint8_t reg,
                          uint8_t* buf, size_t len);

    /**
     * @brief Read register (16-bit register address)
     */
    Result<void> read_reg16(uint8_t dev_addr, uint16_t reg,
                            uint8_t* buf, size_t len);

    /**
     * @brief Raw write (no register address prefix)
     */
    Result<void> write(uint8_t dev_addr, const uint8_t* data, size_t len);

    /**
     * @brief Raw read
     */
    Result<void> read(uint8_t dev_addr, uint8_t* buf, size_t len);

    /**
     * @brief Combined transfer (I2C_RDWR ioctl, supports repeated-start)
     */
    Result<void> transfer(std::vector<Message>& msgs);

    /**
     * @brief Probe whether a device is present (send address, check ACK)
     */
    bool probe(uint8_t dev_addr) noexcept;

    int bus_num() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
