/**
 * @file logger.hpp
 * @brief RV1126B SDK logging interface
 *
 * Provides leveled logging (TRACE/DEBUG/INFO/WARN/ERROR/FATAL) with support for:
 *  - Colored console output
 *  - Rolling file output
 *  - Custom back-end (ILogSink)
 *
 * Quick macro:
 * @code
 * RV_LOG_INFO("camera", "started %dx%d", width, height);
 * @endcode
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <memory>
#include <functional>

namespace rv1126b {

// ─── Log levels ──────────────────────────────────────────────────────────────────

enum class LogLevel : uint8_t {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
    Off   = 6,
};

constexpr const char* to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO ";
        case LogLevel::Warn:  return "WARN ";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default:              return "     ";
    }
}

// ─── Log record structure ────────────────────────────────────────────────────────

struct LogRecord {
    LogLevel    level;
    const char* tag;          ///< Module tag, e.g. "camera", "npu"
    const char* file;
    int         line;
    const char* message;
    int64_t     timestamp_ms;
};

/// Custom log output back-end interface
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const LogRecord& record) = 0;
    virtual void flush() {}
};

// ─── Logger ────────────────────────────────────────────────────────────────────

/**
 * @brief Global logger (singleton)
 *
 * Writes to stderr by default; supports setting log level and adding custom sinks.
 */
class Logger {
public:
    /// Get the global singleton instance
    static Logger& get() noexcept;

    /// Set the global minimum log level
    void set_level(LogLevel level) noexcept;
    LogLevel level() const noexcept;

    /// Add a custom sink
    void add_sink(std::shared_ptr<ILogSink> sink);
    void clear_sinks();

    /// Enable/disable colored console output (enabled by default)
    void enable_console(bool enable);

    /// Enable file output (rolling write)
    void enable_file(const std::string& path,
                     size_t max_size_mb = 10,
                     int    max_files   = 3);

    /// Core write interface (called by macros)
    void log(LogLevel level, const char* tag,
             const char* file, int line, const char* message);

private:
    Logger();
    ~Logger();

    LogLevel level_{LogLevel::Info};

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b

// ─── Logging macros ──────────────────────────────────────────────────────────────

#define _RV_LOG_IMPL(lvl, tag, fmt, ...)                              \
    do {                                                              \
        if (static_cast<int>(::rv1126b::LogLevel::lvl) >=            \
            static_cast<int>(::rv1126b::Logger::get().level())) {    \
            char _buf[2048];                                          \
            snprintf(_buf, sizeof(_buf), fmt, ##__VA_ARGS__);        \
            ::rv1126b::Logger::get().log(                             \
                ::rv1126b::LogLevel::lvl, tag, __FILE__, __LINE__, _buf); \
        }                                                             \
    } while (0)

#define RV_LOG_TRACE(tag, fmt, ...) _RV_LOG_IMPL(Trace, tag, fmt, ##__VA_ARGS__)
#define RV_LOG_DEBUG(tag, fmt, ...) _RV_LOG_IMPL(Debug, tag, fmt, ##__VA_ARGS__)
#define RV_LOG_INFO(tag, fmt, ...)  _RV_LOG_IMPL(Info,  tag, fmt, ##__VA_ARGS__)
#define RV_LOG_WARN(tag, fmt, ...)  _RV_LOG_IMPL(Warn,  tag, fmt, ##__VA_ARGS__)
#define RV_LOG_ERROR(tag, fmt, ...) _RV_LOG_IMPL(Error, tag, fmt, ##__VA_ARGS__)
#define RV_LOG_FATAL(tag, fmt, ...) _RV_LOG_IMPL(Fatal, tag, fmt, ##__VA_ARGS__)
