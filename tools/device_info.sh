#!/bin/bash
# Device info tool for RV1126B
# SSH into device and collect system information
# Usage: ./tools/device_info.sh [device_ip]

set -euo pipefail

# ---------------------------------------------------------------
# Colored output helpers
# ---------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
success() { echo -e "${GREEN}[OK]${RESET}    $*"; }
section() { echo -e "\n${BOLD}${YELLOW}=== $* ===${RESET}"; }

# ---------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------
DEVICE_IP="${1:-${DEVICE_IP:-192.168.x.xx}}"
DEVICE_USER="root"
DEVICE_PASS="recamera.1"
SSH_OPTS="-o StrictHostKeyChecking=no -o ConnectTimeout=10 -o LogLevel=ERROR"

if command -v sshpass &>/dev/null; then
    SSH="sshpass -p '${DEVICE_PASS}' ssh ${SSH_OPTS} ${DEVICE_USER}@${DEVICE_IP}"
else
    SSH="ssh ${SSH_OPTS} ${DEVICE_USER}@${DEVICE_IP}"
fi

run() {
    # Run a command on device and print output
    eval "${SSH} '$*'" 2>/dev/null || echo "  (not available)"
}

# ---------------------------------------------------------------
# Header
# ---------------------------------------------------------------
echo -e "${BOLD}============================================${RESET}"
echo -e "${BOLD}  RV1126B Device Info                      ${RESET}"
echo -e "${BOLD}============================================${RESET}"
info "Device: ${DEVICE_USER}@${DEVICE_IP}"
echo ""

# ---------------------------------------------------------------
# Test connectivity
# ---------------------------------------------------------------
if ! eval "${SSH} 'true'" &>/dev/null; then
    echo -e "${RED}[ERROR]${RESET} Cannot connect to ${DEVICE_IP}. Check IP and network." >&2
    exit 1
fi
success "Connection established"

# ---------------------------------------------------------------
# Kernel and system
# ---------------------------------------------------------------
section "Kernel and System"
echo "Kernel version:"
run "uname -a"
echo ""
echo "OS release:"
run "cat /etc/os-release 2>/dev/null || cat /etc/issue 2>/dev/null"
echo ""
echo "Uptime:"
run "uptime"

# ---------------------------------------------------------------
# CPU information
# ---------------------------------------------------------------
section "CPU Information"
run "cat /proc/cpuinfo | grep -E '^(processor|model name|Hardware|Revision|BogoMIPS)' | head -30"

# ---------------------------------------------------------------
# Memory
# ---------------------------------------------------------------
section "Memory"
run "free -h"
echo ""
echo "Memory details:"
run "cat /proc/meminfo | head -20"

# ---------------------------------------------------------------
# Storage
# ---------------------------------------------------------------
section "Storage"
run "df -h"

# ---------------------------------------------------------------
# Temperature sensors
# ---------------------------------------------------------------
section "Temperatures"
echo "Thermal zones:"
run "for f in /sys/class/thermal/thermal_zone*/; do
    zone=\$(basename \$f)
    type=\$(cat \$f/type 2>/dev/null || echo 'unknown')
    temp=\$(cat \$f/temp 2>/dev/null || echo '0')
    temp_c=\$(echo \"scale=1; \$temp / 1000\" | bc 2>/dev/null || echo \"\$temp\")
    echo \"  \$zone (\$type): \${temp_c} °C\"
done"

echo ""
echo "Rockchip CPU temperature:"
run "cat /sys/class/thermal/thermal_zone0/temp | awk '{printf \"%.1f °C\n\", \$1/1000}'"

# ---------------------------------------------------------------
# Running processes (relevant to camera/media)
# ---------------------------------------------------------------
section "Running Processes"
echo "Top processes by CPU:"
run "top -bn1 | head -20"
echo ""
echo "Media/camera related processes:"
run "ps aux | grep -E '(isp|camera|rkisp|rkcif|mpp|rknn|rtsp|onvif)' | grep -v grep || echo '  (none found)'"

# ---------------------------------------------------------------
# Rockchip libraries
# ---------------------------------------------------------------
section "Rockchip Libraries in /oem/usr/lib/"
run "ls -lh /oem/usr/lib/*.so* 2>/dev/null | sort || echo '  (directory not found)'"

echo ""
echo "NPU / RKNN libraries:"
run "find /oem /usr -name 'librknn*' -o -name 'librknpu*' 2>/dev/null | sort"

echo ""
echo "MPP libraries:"
run "find /oem /usr -name 'librockchip_mpp*' 2>/dev/null | sort"

# ---------------------------------------------------------------
# Video devices (V4L2)
# ---------------------------------------------------------------
section "Video Devices (V4L2)"
run "ls -la /dev/video* 2>/dev/null || echo '  (none found)'"
echo ""
echo "V4L2 device details:"
run "for dev in /dev/video*; do
    [ -e \"\$dev\" ] || continue
    echo \"  \$dev:\"
    v4l2-ctl -d \"\$dev\" --info 2>/dev/null | grep -E '(Driver|Card|Bus)' | sed 's/^/    /' || true
done"

# ---------------------------------------------------------------
# GPIO
# ---------------------------------------------------------------
section "GPIO Chips"
run "ls /sys/class/gpio/ 2>/dev/null || echo '  (none found)'"
echo ""
run "for chip in /sys/class/gpio/gpiochip*; do
    [ -e \"\$chip\" ] || continue
    label=\$(cat \$chip/label 2>/dev/null || echo 'unknown')
    base=\$(cat \$chip/base 2>/dev/null || echo '?')
    ngpio=\$(cat \$chip/ngpio 2>/dev/null || echo '?')
    echo \"  \$(basename \$chip): label=\$label  base=\$base  ngpio=\$ngpio\"
done"

# ---------------------------------------------------------------
# I2C buses
# ---------------------------------------------------------------
section "I2C Buses"
run "ls /dev/i2c-* 2>/dev/null || echo '  (none found)'"
echo ""
echo "Detected I2C devices (i2cdetect):"
run "for bus in /dev/i2c-*; do
    [ -e \"\$bus\" ] || continue
    n=\${bus#/dev/i2c-}
    echo \"  Bus \$n (\$bus):\"
    i2cdetect -y \$n 2>/dev/null | sed 's/^/    /' || echo '    (i2cdetect not available)'
done"

# ---------------------------------------------------------------
# IIO devices (IMU, ADC)
# ---------------------------------------------------------------
section "IIO Devices (IMU / ADC)"
run "ls /sys/bus/iio/devices/ 2>/dev/null || echo '  (none found)'"
echo ""
run "for dev in /sys/bus/iio/devices/iio:device*; do
    [ -e \"\$dev\" ] || continue
    name=\$(cat \$dev/name 2>/dev/null || echo 'unknown')
    echo \"  \$(basename \$dev): \$name\"
    ls \$dev/in_*_raw 2>/dev/null | sed 's/^/    /' | head -10 || true
done"

# ---------------------------------------------------------------
# UART devices
# ---------------------------------------------------------------
section "UART Devices"
run "ls -la /dev/ttyS* /dev/ttyAMA* 2>/dev/null || echo '  (none found)'"

# ---------------------------------------------------------------
# Network interfaces
# ---------------------------------------------------------------
section "Network Interfaces"
run "ip addr show"
echo ""
echo "Default route:"
run "ip route show default"

# ---------------------------------------------------------------
# RKNN model files
# ---------------------------------------------------------------
section "RKNN Model Files"
run "find /oem/usr/share/model /oem/usr/share/rknn 2>/dev/null \
    -name '*.rknn' | sort | head -20 || echo '  (no models found)'"

# ---------------------------------------------------------------
# ISP IQ files
# ---------------------------------------------------------------
section "ISP IQ Files"
run "ls /oem/usr/share/iqfiles/ 2>/dev/null | head -20 || echo '  (directory not found)'"

echo ""
success "Device info collection complete."
