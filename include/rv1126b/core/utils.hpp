/**
 * @file utils.hpp
 * @brief RV1126B SDK utility functions and common types
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include "rv1126b/core/error.hpp"

namespace rv1126b {

// ─── Time utilities ──────────────────────────────────────────────────────────────

/// Return current timestamp in milliseconds (monotonic clock)
inline int64_t now_ms() noexcept {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
}

/// Return current timestamp in microseconds (monotonic clock)
inline int64_t now_us() noexcept {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}

/// Sleep for the specified number of milliseconds
void sleep_ms(int ms) noexcept;

// ─── Data buffer ─────────────────────────────────────────────────────────────────

/// Non-copyable memory block (RAII)
class Buffer {
public:
    Buffer() = default;
    explicit Buffer(size_t size);
    Buffer(const void* data, size_t size);
    ~Buffer();

    Buffer(Buffer&&) noexcept;
    Buffer& operator=(Buffer&&) noexcept;

    Buffer(const Buffer&)            = delete;
    Buffer& operator=(const Buffer&) = delete;

    uint8_t*       data()       noexcept { return data_; }
    const uint8_t* data() const noexcept { return data_; }
    size_t         size() const noexcept { return size_; }
    bool           empty() const noexcept { return size_ == 0; }

    void resize(size_t new_size);
    void reset();

private:
    uint8_t* data_{nullptr};
    size_t   size_{0};
};

// ─── Image size ──────────────────────────────────────────────────────────────────

struct Size {
    int width{0};
    int height{0};

    bool operator==(const Size& o) const noexcept {
        return width == o.width && height == o.height;
    }
    bool operator!=(const Size& o) const noexcept { return !(*this == o); }
};

// ─── Rectangle region ────────────────────────────────────────────────────────────

struct Rect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

// ─── Pixel format ────────────────────────────────────────────────────────────────

enum class PixelFormat {
    Unknown  = 0,
    NV12,       ///< YUV 4:2:0, semi-planar (most common on RV1126B)
    NV21,
    YUV420P,
    RGB888,
    BGR888,
    RGBA8888,
    BGRA8888,
    MJPEG,
};

constexpr const char* to_string(PixelFormat fmt) noexcept {
    switch (fmt) {
        case PixelFormat::NV12:    return "NV12";
        case PixelFormat::NV21:    return "NV21";
        case PixelFormat::YUV420P: return "YUV420P";
        case PixelFormat::RGB888:  return "RGB888";
        case PixelFormat::BGR888:  return "BGR888";
        case PixelFormat::RGBA8888:return "RGBA8888";
        case PixelFormat::BGRA8888:return "BGRA8888";
        case PixelFormat::MJPEG:   return "MJPEG";
        default:                   return "Unknown";
    }
}

// ─── Codec type ──────────────────────────────────────────────────────────────────

enum class CodecType {
    H264  = 0,
    H265  = 1,
    MJPEG = 2,
    JPEG  = 3,
};

constexpr const char* to_string(CodecType c) noexcept {
    switch (c) {
        case CodecType::H264:  return "H264";
        case CodecType::H265:  return "H265";
        case CodecType::MJPEG: return "MJPEG";
        case CodecType::JPEG:  return "JPEG";
        default:               return "Unknown";
    }
}

// ─── Video frame ─────────────────────────────────────────────────────────────────

struct VideoFrame {
    uint8_t* data{nullptr};   ///< Frame data pointer (managed internally by SDK — do not free)
    size_t   size{0};         ///< Frame data byte count
    int64_t  pts_us{0};       ///< Presentation timestamp (microseconds)
    int64_t  dts_us{0};       ///< Decode timestamp (microseconds)
    bool     is_keyframe{false};
    int      width{0};
    int      height{0};
    CodecType codec{CodecType::H264};
    PixelFormat pixel_format{PixelFormat::NV12};
};

/// Frame callback: data is only valid during the callback; copy it if you need to retain it
using FrameCallback = std::function<void(const VideoFrame& frame)>;

// ─── File/path utilities ─────────────────────────────────────────────────────────

/// Check whether a file exists
bool file_exists(const std::string& path) noexcept;

/// Read an entire file into a buffer
Result<Buffer> read_file(const std::string& path);

/// Write a buffer to a file
Result<void> write_file(const std::string& path, const uint8_t* data, size_t size);

/// Read sysfs file content (strips trailing newline)
Result<std::string> sysfs_read(const std::string& path);

/// Write to a sysfs file
Result<void> sysfs_write(const std::string& path, const std::string& value);

// ─── String utilities ────────────────────────────────────────────────────────────

/// Strip leading and trailing whitespace
std::string trim(const std::string& s);

/// Split a string by delimiter
std::vector<std::string> split(const std::string& s, char delim);

} // namespace rv1126b
