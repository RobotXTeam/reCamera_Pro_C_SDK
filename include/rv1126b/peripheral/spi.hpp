/**
 * @file spi.hpp
 * @brief RV1126B SDK — SPI module
 *
 * Based on Linux spidev (/dev/spidevN.M) interface.
 * RV1126B exposes SPI0/SPI1; available after configuring spidev in DTS.
 *
 * @code
 * SPI spi;
 * SPI::Config cfg;
 * cfg.speed_hz    = 10000000;  // 10 MHz
 * cfg.mode        = SPI::Mode::Mode0;
 * cfg.bits_per_word = 8;
 * spi.open("/dev/spidev0.0", cfg);
 *
 * uint8_t tx[4] = {0x01, 0x02, 0x03, 0x04};
 * uint8_t rx[4] = {};
 * spi.transfer(tx, rx, 4);
 * spi.close();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace rv1126b {

class SPI {
public:
    enum class Mode : uint8_t {
        Mode0 = 0,   ///< CPOL=0, CPHA=0
        Mode1 = 1,   ///< CPOL=0, CPHA=1
        Mode2 = 2,   ///< CPOL=1, CPHA=0
        Mode3 = 3,   ///< CPOL=1, CPHA=1
    };

    enum class BitOrder : uint8_t {
        MSBFirst = 0,
        LSBFirst = 1,
    };

    struct Config {
        uint32_t  speed_hz      {1000000};      ///< Clock frequency (Hz), default 1 MHz
        Mode      mode          {Mode::Mode0};
        BitOrder  bit_order     {BitOrder::MSBFirst};
        uint8_t   bits_per_word {8};
        bool      cs_active_high{false};         ///< CS default active low
    };

    SPI();
    ~SPI();

    SPI(const SPI&)            = delete;
    SPI& operator=(const SPI&) = delete;
    SPI(SPI&&) noexcept;
    SPI& operator=(SPI&&) noexcept;

    /**
     * @brief Open the SPI device
     * @param device  Device path, e.g. "/dev/spidev0.0"
     * @param config  SPI parameters
     */
    Result<void> open(const std::string& device, const Config& config = {});

    /// Close the device
    void close();

    bool is_open() const noexcept;

    /**
     * @brief Full-duplex transfer
     * @param tx   Transmit buffer (can be nullptr to send all zeros)
     * @param rx   Receive buffer (can be nullptr to ignore received data)
     * @param len  Byte count to transfer
     */
    Result<void> transfer(const uint8_t* tx, uint8_t* rx, size_t len);

    /**
     * @brief Write only (half-duplex)
     */
    Result<void> write(const uint8_t* data, size_t len);

    /**
     * @brief Read only (half-duplex, transmits all zeros)
     */
    Result<void> read(uint8_t* buf, size_t len);

    /**
     * @brief Write then read (write tx_buf first, then read rx_len bytes using repeated-start)
     */
    Result<void> write_then_read(const uint8_t* tx, size_t tx_len,
                                 uint8_t* rx, size_t rx_len);

    /// Change speed at runtime
    Result<void> set_speed(uint32_t speed_hz);

    /// Change mode at runtime
    Result<void> set_mode(Mode mode);

    const Config& config() const noexcept;
    const std::string& device() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
