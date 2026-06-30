/**
 * @file config.hpp
 * @brief RV1126B SDK configuration file read/write interface
 *
 * Supports JSON/INI formats with a unified key-value read/write interface.
 *
 * @code
 * Config cfg;
 * cfg.load("/etc/myapp/config.json");
 * int width  = cfg.get<int>("camera.width", 1920);
 * float gain = cfg.get<float>("isp.gain",   1.0f);
 * cfg.set("camera.width", 2560);
 * cfg.save();
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <string>
#include <optional>
#include <memory>

namespace rv1126b {

class Config {
public:
    Config();
    ~Config();

    /// Load configuration from file (supports .json / .ini)
    Result<void> load(const std::string& path);

    /// Save configuration to file (retain loaded path, or specify a new one)
    Result<void> save(const std::string& path = "");

    /// Read a config value; returns default_val if the key does not exist
    template<typename T>
    T get(const std::string& key, const T& default_val = T{}) const;

    /// Write a config value
    template<typename T>
    void set(const std::string& key, const T& value);

    /// Check whether a key exists
    bool has(const std::string& key) const;

    /// Delete a key
    void remove(const std::string& key);

    /// Get the currently loaded file path
    const std::string& path() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rv1126b
