/**
 * @file error.hpp
 * @brief RV1126B SDK error codes and Result<T> type
 *
 * Provides a unified error representation:
 *  - Error enum: covers error codes for all modules
 *  - Result<T>: lightweight expected-like wrapper that avoids exception overhead
 *
 * Usage example:
 * @code
 * Result<int> val = some_function();
 * if (!val) {
 *     logger.error("failed: {}", to_string(val.error()));
 *     return val.error();
 * }
 * int x = *val;
 * @endcode
 */
#pragma once

#include <cstdint>
#include <string_view>
#include <cassert>
#include <utility>
#include <variant>

namespace rv1126b {

// ─── Error codes ─────────────────────────────────────────────────────────────────

enum class Error : int32_t {
    Ok              =  0,
    Unknown         = -1,
    InvalidParam    = -2,
    NoMemory        = -3,
    IoError         = -4,
    Timeout         = -5,
    NotFound        = -6,
    Busy            = -7,
    NotInitialized  = -8,
    AlreadyRunning  = -9,
    NotSupported    = -10,
    BufferTooSmall  = -11,
    PermissionDenied= -12,
    DeviceError     = -13,
    NetworkError    = -14,
    CodecError      = -15,
    ModelError      = -16,
    InferenceError  = -17,
};

/// Convert error code to human-readable string
constexpr std::string_view to_string(Error e) noexcept {
    switch (e) {
        case Error::Ok:              return "Ok";
        case Error::Unknown:         return "Unknown";
        case Error::InvalidParam:    return "InvalidParam";
        case Error::NoMemory:        return "NoMemory";
        case Error::IoError:         return "IoError";
        case Error::Timeout:         return "Timeout";
        case Error::NotFound:        return "NotFound";
        case Error::Busy:            return "Busy";
        case Error::NotInitialized:  return "NotInitialized";
        case Error::AlreadyRunning:  return "AlreadyRunning";
        case Error::NotSupported:    return "NotSupported";
        case Error::BufferTooSmall:  return "BufferTooSmall";
        case Error::PermissionDenied:return "PermissionDenied";
        case Error::DeviceError:     return "DeviceError";
        case Error::NetworkError:    return "NetworkError";
        case Error::CodecError:      return "CodecError";
        case Error::ModelError:      return "ModelError";
        case Error::InferenceError:  return "InferenceError";
        default:                     return "UnknownError";
    }
}

// ─── Result<T> ───────────────────────────────────────────────────────────────────

/// Lightweight Result type holding either a success value or an error code
template<typename T>
class Result {
public:
    // Success constructor
    Result(T value) : data_(std::move(value)) {}  // NOLINT(google-explicit-constructor)

    // Error constructor
    Result(Error err) : data_(err) {}  // NOLINT(google-explicit-constructor)

    /// Whether the result is a success
    [[nodiscard]] bool ok() const noexcept {
        return std::holds_alternative<T>(data_);
    }
    explicit operator bool() const noexcept { return ok(); }

    /// Retrieve the success value (caller must check ok() first)
    T& value() {
        assert(ok() && "Result::value() called on error result");
        return std::get<T>(data_);
    }
    const T& value() const {
        assert(ok() && "Result::value() called on error result");
        return std::get<T>(data_);
    }
    T& operator*()       { return value(); }
    const T& operator*() const { return value(); }
    T* operator->()      { return &value(); }
    const T* operator->() const { return &value(); }

    /// Retrieve the error code (caller must ensure !ok() first)
    [[nodiscard]] Error error() const noexcept {
        assert(!ok() && "Result::error() called on success result");
        return std::get<Error>(data_);
    }

    /// Return the value or a default if error
    T value_or(T&& def) const {
        return ok() ? std::get<T>(data_) : std::forward<T>(def);
    }

private:
    std::variant<T, Error> data_;
};

/// Result<void> specialization: represents success/failure only
template<>
class Result<void> {
public:
    Result() : err_(Error::Ok) {}
    Result(Error err) : err_(err) {}  // NOLINT(google-explicit-constructor)

    [[nodiscard]] bool ok() const noexcept { return err_ == Error::Ok; }
    explicit operator bool() const noexcept { return ok(); }
    [[nodiscard]] Error error() const noexcept { return err_; }

private:
    Error err_;
};

/// Successful Result<void>
inline Result<void> Ok() { return {}; }

/// Failed Result<void>
inline Result<void> Err(Error e) { return e; }

} // namespace rv1126b
