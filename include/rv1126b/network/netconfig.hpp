/**
 * @file netconfig.hpp
 * @brief RV1126B SDK — network configuration module
 *
 * Provides network interface information query and configuration.
 * Uses standard Linux network ioctls such as ioctl(SIOCGIFADDR).
 *
 * @code
 * auto info = net::get_info("eth0");
 * if (info) {
 *     printf("IP: %s, MAC: %s\n",
 *            info->ip.c_str(), info->mac.c_str());
 * }
 * @endcode
 */
#pragma once

#include "rv1126b/core/error.hpp"
#include <string>
#include <vector>

namespace rv1126b {

struct NetworkInfo {
    std::string interface;     ///< Interface name, e.g. "eth0", "wlan0"
    std::string ip;            ///< IPv4 address (dotted decimal)
    std::string netmask;       ///< Subnet mask
    std::string broadcast;     ///< Broadcast address
    std::string gateway;       ///< Default gateway (read from /proc/net/route)
    std::string mac;           ///< MAC address (colon-separated hex)
    bool        is_up    {false};
    bool        is_running{false};
    int         mtu      {1500};
};

struct WifiInfo {
    std::string ssid;
    std::string bssid;
    int         signal_dbm    {0};
    int         freq_mhz      {0};
    std::string security;
    bool        connected     {false};
};

namespace net {

/**
 * @brief Get network info for the specified interface
 * @param iface Interface name, e.g. "eth0" (empty string returns first available)
 */
Result<NetworkInfo> get_info(const std::string& iface = "eth0");

/**
 * @brief Enumerate all network interfaces
 */
Result<std::vector<NetworkInfo>> list_interfaces();

/**
 * @brief Statically set an IP address (via ioctl SIOCSIFADDR)
 * Note: requires root. Takes effect immediately; lost on reboot unless written to /etc/network/interfaces.
 * @param iface    Interface name
 * @param ip       IPv4 address (dotted decimal)
 * @param netmask  Subnet mask (dotted decimal)
 */
Result<void> set_ip(const std::string& iface,
                    const std::string& ip,
                    const std::string& netmask);

/**
 * @brief Set the default gateway (via ioctl SIOCADDRT)
 */
Result<void> set_gateway(const std::string& gateway);

/**
 * @brief Get current WiFi connection info (reads /proc/net/wireless or iwconfig)
 */
Result<WifiInfo> get_wifi_info(const std::string& iface = "wlan0");

/**
 * @brief Scan WiFi networks (via iwlist scan; requires wlan interface)
 */
Result<std::vector<WifiInfo>> scan_wifi(const std::string& iface = "wlan0");

/**
 * @brief Connect to WiFi (invokes wpa_supplicant CLI)
 * @param iface    wlan interface name
 * @param ssid     WiFi SSID
 * @param password WPA2 password (empty string for open network)
 */
Result<void> connect_wifi(const std::string& iface,
                          const std::string& ssid,
                          const std::string& password = "");

/**
 * @brief Disconnect from WiFi
 */
Result<void> disconnect_wifi(const std::string& iface = "wlan0");

} // namespace net
} // namespace rv1126b
