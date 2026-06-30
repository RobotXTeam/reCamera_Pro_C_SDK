/**
 * @file audio.hpp
 * @brief RV1126B SDK — audio input/output module
 *
 * Based on Linux ALSA (Advanced Linux Sound Architecture).
 * Uses /dev/snd/ devices and standard ALSA PCM ioctls for recording and playback.
 * No external dependencies (does not use libasound; uses kernel ALSA ioctls directly).
 *
 * @code
 * // Record audio
 * AudioIn ain;
 * AudioIn::Config cfg{.sample_rate=48000, .channels=1, .bits=16};
 * ain.init("hw:0,0", cfg);
 * std::vector<int16_t> buf(cfg.sample_rate);  // 1 second
 * ain.read(buf.data(), buf.size() * 2);
 * ain.deinit();
 *
 * // Playback
 * AudioOut aout;
 * aout.init("hw:0,0", cfg);
 * aout.write(audio_data, audio_size);
 * aout.deinit();
 * @endcode
 */
#pragma once

#include <memory>

#include "rv1126b/core/error.hpp"
#include <cstdint>
#include <string>

namespace rv1126b {

struct AudioConfig {
    int sample_rate {48000};   ///< Sample rate (Hz), e.g. 8000, 16000, 44100, 48000
    int channels    {1};       ///< Channel count: 1=mono, 2=stereo
    int bits        {16};      ///< Sample bit depth: 8, 16, 32
    int period_frames{1024};   ///< Frames per period (affects latency)
    int periods     {4};       ///< Number of periods in the buffer
};

// ─── Audio input (recording) ─────────────────────────────────────────────────────

class AudioIn {
public:
    AudioIn();
    ~AudioIn();

    AudioIn(const AudioIn&)            = delete;
    AudioIn& operator=(const AudioIn&) = delete;
    AudioIn(AudioIn&&) noexcept;
    AudioIn& operator=(AudioIn&&) noexcept;

    /**
     * @brief Initialize ALSA recording device
     * @param device ALSA device name, e.g. "hw:0,0" or "default"
     * @param config Audio parameters
     */
    Result<void> init(const std::string& device = "hw:0,0",
                      const AudioConfig& config  = {});

    /// Release resources
    void deinit();

    bool is_initialized() const noexcept;

    /**
     * @brief Read PCM data
     * @param buf     Receive buffer
     * @param size    Maximum byte count (recommended: integer multiple of period_frames * channels * (bits/8))
     * @return Actual bytes read
     */
    Result<size_t> read(void* buf, size_t size);

    /**
     * @brief Start recording (device is ready after init; this manually starts it)
     */
    Result<void> start();

    /// Stop recording
    Result<void> stop();

    const AudioConfig& config() const noexcept;
    const std::string& device() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ─── Audio output (playback) ─────────────────────────────────────────────────────

class AudioOut {
public:
    AudioOut();
    ~AudioOut();

    AudioOut(const AudioOut&)            = delete;
    AudioOut& operator=(const AudioOut&) = delete;
    AudioOut(AudioOut&&) noexcept;
    AudioOut& operator=(AudioOut&&) noexcept;

    /**
     * @brief Initialize ALSA playback device
     * @param device ALSA device name, e.g. "hw:0,0" or "default"
     * @param config Audio parameters
     */
    Result<void> init(const std::string& device = "hw:0,0",
                      const AudioConfig& config  = {});

    /// Release resources
    void deinit();

    bool is_initialized() const noexcept;

    /**
     * @brief Write PCM data (blocks until write completes or buffer is full)
     * @param data  PCM audio data
     * @param size  Byte count
     * @return Actual bytes written
     */
    Result<size_t> write(const void* data, size_t size);

    /**
     * @brief Wait until the playback buffer drains
     */
    Result<void> drain();

    /**
     * @brief Immediately stop playback and discard the buffer
     */
    Result<void> drop();

    /// Set playback volume (0-100)
    Result<void> set_volume(int percent);

    const AudioConfig& config() const noexcept;
    const std::string& device() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
