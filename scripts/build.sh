#!/bin/bash
# =============================================================================
# reCamera Pro C++ SDK — Build Script
#
# Usage: ./scripts/build.sh [release|debug] [--examples] [--tests] [--clean]
#
# Workspace layout expected (same level as this SDK repo):
#   <workspace>/
#     reCamera_Pro_C++_SDK/           <- this repo (you are here)
#     reCamera_RV1126B_Toolchain/     <- toolchain (download from Releases)
#
# Environment variables (override auto-detection):
#   RV1126B_TOOLCHAIN_DIR   Explicit path to cross-compiler bin/
#   RV1126B_PLATFORM_SDK    Path to platform SDK root (default: ./sysroot)
# =============================================================================
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WORKSPACE="$(cd "${SDK_ROOT}/.." && pwd)"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
info()  { echo -e "${CYAN}[INFO]${NC}  $*"; }
ok()    { echo -e "${GREEN}[ OK ]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
fail()  { echo -e "${RED}[FAIL]${NC}  $*"; exit 1; }

BUILD_TYPE="Release"; BUILD_EXAMPLES="OFF"; BUILD_TESTS="OFF"; DO_CLEAN=0
JOBS=$(nproc 2>/dev/null || echo 4)

for arg in "$@"; do
    case "$arg" in
        release|Release) BUILD_TYPE="Release" ;;
        debug|Debug)     BUILD_TYPE="Debug" ;;
        --examples)      BUILD_EXAMPLES="ON" ;;
        --tests)         BUILD_TESTS="ON" ;;
        --clean)         DO_CLEAN=1 ;;
        --jobs=*)        JOBS="${arg#--jobs=}" ;;
    esac
done

BUILD_DIR="${SDK_ROOT}/build-aarch64"

# ── Toolchain detection ──────────────────────────────────────────────────────
if [ -n "${RV1126B_TOOLCHAIN_DIR}" ]; then
    info "Toolchain [env]: ${RV1126B_TOOLCHAIN_DIR}"

elif [ -d "${WORKSPACE}/reCamera_RV1126B_Toolchain" ]; then
    # Auto-detect sibling directory (Option A)
    TC_SIBLING=$(find "${WORKSPACE}/reCamera_RV1126B_Toolchain" \
        -name "aarch64-rockchip1240-linux-gnu-gcc" 2>/dev/null | head -1)
    if [ -n "$TC_SIBLING" ]; then
        export RV1126B_TOOLCHAIN_DIR="$(dirname "$TC_SIBLING")"
        info "Toolchain [sibling]: ${RV1126B_TOOLCHAIN_DIR}"
    fi

elif command -v aarch64-linux-gnu-gcc &>/dev/null; then
    info "Toolchain [system]: $(which aarch64-linux-gnu-gcc)"
else
    fail "AArch64 cross-compiler not found.\n\n  Option A: Place toolchain at:\n    ${WORKSPACE}/reCamera_RV1126B_Toolchain/\n  Download from: https://github.com/your-org/reCamera-Pro-SDK/releases\n\n  Option B: export RV1126B_TOOLCHAIN_DIR=/path/to/bin\n\n  Option C: sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu"
fi

# ── Platform SDK ─────────────────────────────────────────────────────────────
if [ -n "${RV1126B_PLATFORM_SDK}" ]; then
    info "Platform SDK [env]: ${RV1126B_PLATFORM_SDK}"
elif [ -d "${SDK_ROOT}/sysroot/oem/usr/lib" ]; then
    info "Platform SDK [bundled]: ${SDK_ROOT}/sysroot"
else
    warn "sysroot/ not found. Set RV1126B_PLATFORM_SDK or re-clone the SDK repo."
fi

[ "${DO_CLEAN}" -eq 1 ] && { info "Cleaning..."; rm -rf "${BUILD_DIR}"; }
mkdir -p "${BUILD_DIR}"

info "Configuring CMake (${BUILD_TYPE})..."
cmake -B "${BUILD_DIR}" -S "${SDK_ROOT}" \
    -DCMAKE_TOOLCHAIN_FILE="${SDK_ROOT}/cmake/toolchain-aarch64.cmake" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DRV1126B_BUILD_EXAMPLES="${BUILD_EXAMPLES}" \
    -DRV1126B_BUILD_TESTS="${BUILD_TESTS}" \
    2>&1 | grep -E "^--|error:|FAIL" | head -20

info "Building (j${JOBS})..."
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

echo ""
ok "Build complete!"
echo "  Shared lib : ${BUILD_DIR}/src/libreCamera_pro_sdk.so"
echo "  Static lib : ${BUILD_DIR}/src/libreCamera_pro_sdk.a"
[ "${BUILD_EXAMPLES}" = "ON" ] && echo "  Examples   : ${BUILD_DIR}/examples/"
echo ""
echo "  Deploy SDK lib (first time):"
echo "    scp ${BUILD_DIR}/src/libreCamera_pro_sdk.so.1.0.0 root@192.168.x.xx:/tmp/"
echo "    ssh root@192.168.x.xx 'cd /tmp && ln -sf libreCamera_pro_sdk.so.1.0.0 libreCamera_pro_sdk.so.1'"
