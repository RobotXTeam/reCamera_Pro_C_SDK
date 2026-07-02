#!/bin/bash
# Script to build private driver source and sync to public lib

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PRIVATE_DIR="${SDK_ROOT}/private_src"

if [ ! -d "${PRIVATE_DIR}" ]; then
    echo "[ERROR] private_src directory not found!"
    echo "Please ensure you have created the symlink:"
    echo "  cd ${SDK_ROOT} && ln -s ../reCamera_Pro_Driver_Core private_src"
    exit 1
fi

echo "==> [1/2] Building Private Driver Source..."
cd "${PRIVATE_DIR}"
# Build using the script in the private repo
./scripts/build.sh release

echo "==> [2/2] Syncing updated libraries to public SDK..."
# Use -a to preserve symlinks
cp -a build-aarch64/src/libreCamera_pro_sdk.so* "${SDK_ROOT}/lib/"
cp -a build-aarch64/src/libreCamera_pro_sdk.a "${SDK_ROOT}/lib/" 2>/dev/null || true

echo "==> Update complete!"
