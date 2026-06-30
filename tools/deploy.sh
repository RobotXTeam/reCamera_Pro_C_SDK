#!/bin/bash
# Deploy SDK to RV1126B device
# Usage: ./tools/deploy.sh [binary_or_directory] [device_ip]
#
# Examples:
#   ./tools/deploy.sh build-aarch64/examples/hello_world
#   ./tools/deploy.sh build-aarch64/examples/hello_world 192.168.x.xx
#   ./tools/deploy.sh build-aarch64/lib/ 192.168.x.xx
#   RUN=1 ./tools/deploy.sh build-aarch64/examples/hello_world

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
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
error()   { echo -e "${RED}[ERROR]${RESET} $*" >&2; }
fatal()   { error "$*"; exit 1; }

# ---------------------------------------------------------------
# Default configuration
# ---------------------------------------------------------------
DEFAULT_DEVICE_IP="192.168.x.xx"
DEVICE_USER="root"
DEVICE_PASS="recamera.1"
REMOTE_DIR="/tmp"
SSH_OPTS="-o StrictHostKeyChecking=no -o ConnectTimeout=10 -o LogLevel=ERROR"
# Set RUN=1 to automatically execute the binary after deployment
RUN="${RUN:-0}"

# ---------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------
if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <binary_or_directory> [device_ip]"
    echo ""
    echo "Environment variables:"
    echo "  DEVICE_IP    Target device IP (default: ${DEFAULT_DEVICE_IP})"
    echo "  DEVICE_PASS  SSH password     (default: ${DEVICE_PASS})"
    echo "  REMOTE_DIR   Remote directory (default: ${REMOTE_DIR})"
    echo "  RUN          Set to 1 to run after deploy"
    exit 1
fi

LOCAL_PATH="${1}"
DEVICE_IP="${2:-${DEVICE_IP:-${DEFAULT_DEVICE_IP}}}"

echo -e "${BOLD}============================================${RESET}"
echo -e "${BOLD}  RV1126B SDK Deploy Tool                  ${RESET}"
echo -e "${BOLD}============================================${RESET}"
info "Source      : ${LOCAL_PATH}"
info "Target      : ${DEVICE_USER}@${DEVICE_IP}:${REMOTE_DIR}"
echo ""

# ---------------------------------------------------------------
# Verify source exists
# ---------------------------------------------------------------
if [[ ! -e "${LOCAL_PATH}" ]]; then
    fatal "Source not found: ${LOCAL_PATH}"
fi

# ---------------------------------------------------------------
# Check sshpass is available (needed for password auth)
# ---------------------------------------------------------------
SCP_CMD="scp"
SSH_CMD="ssh"

if command -v sshpass &>/dev/null; then
    SCP_CMD="sshpass -p '${DEVICE_PASS}' scp"
    SSH_CMD="sshpass -p '${DEVICE_PASS}' ssh"
    info "Using sshpass for password authentication"
else
    warn "sshpass not found. You will be prompted for the password."
    warn "Install with: sudo apt-get install -y sshpass"
fi

# ---------------------------------------------------------------
# Test SSH connectivity
# ---------------------------------------------------------------
info "Testing connection to ${DEVICE_IP}..."
if ! eval "${SSH_CMD} ${SSH_OPTS} ${DEVICE_USER}@${DEVICE_IP} 'echo connected'" &>/dev/null; then
    fatal "Cannot connect to ${DEVICE_USER}@${DEVICE_IP}. Check IP and network."
fi
success "Connection OK"

# ---------------------------------------------------------------
# Copy files
# ---------------------------------------------------------------
if [[ -f "${LOCAL_PATH}" ]]; then
    # Single file
    REMOTE_DEST="${REMOTE_DIR}/$(basename "${LOCAL_PATH}")"
    info "Copying $(basename "${LOCAL_PATH}") -> ${DEVICE_USER}@${DEVICE_IP}:${REMOTE_DEST}..."

    eval "${SCP_CMD} ${SSH_OPTS} '${LOCAL_PATH}' \
        '${DEVICE_USER}@${DEVICE_IP}:${REMOTE_DEST}'"

    # Set executable permission
    eval "${SSH_CMD} ${SSH_OPTS} ${DEVICE_USER}@${DEVICE_IP} \
        'chmod +x ${REMOTE_DEST} 2>/dev/null || true'"

    success "Deployed: ${REMOTE_DEST}"

elif [[ -d "${LOCAL_PATH}" ]]; then
    # Directory — copy recursively
    info "Copying directory $(basename "${LOCAL_PATH}")/ -> ${DEVICE_USER}@${DEVICE_IP}:${REMOTE_DIR}/..."

    eval "${SCP_CMD} -r ${SSH_OPTS} '${LOCAL_PATH}' \
        '${DEVICE_USER}@${DEVICE_IP}:${REMOTE_DIR}/'"

    # Make all executables in the directory executable
    REMOTE_SUBDIR="${REMOTE_DIR}/$(basename "${LOCAL_PATH}")"
    eval "${SSH_CMD} ${SSH_OPTS} ${DEVICE_USER}@${DEVICE_IP} \
        'find ${REMOTE_SUBDIR} -type f -exec file {} + | grep ELF | cut -d: -f1 \
         | xargs chmod +x 2>/dev/null || true'"

    success "Deployed directory: ${REMOTE_SUBDIR}"
    REMOTE_DEST="${REMOTE_DIR}/$(basename "${LOCAL_PATH}")"
fi

# ---------------------------------------------------------------
# Optionally run the binary
# ---------------------------------------------------------------
if [[ "${RUN}" == "1" && -f "${LOCAL_PATH}" ]]; then
    echo ""
    info "Running ${REMOTE_DEST} on device..."
    echo -e "${BOLD}---- Device Output ----${RESET}"
    eval "${SSH_CMD} ${SSH_OPTS} ${DEVICE_USER}@${DEVICE_IP} \
        'LD_LIBRARY_PATH=/oem/usr/lib:/usr/lib:${LD_LIBRARY_PATH:-} \
         ${REMOTE_DEST}'"
    echo -e "${BOLD}---- End of Output ----${RESET}"
fi

echo ""
echo -e "${BOLD}To run manually:${RESET}"
echo "  ssh ${DEVICE_USER}@${DEVICE_IP}"
if [[ -f "${LOCAL_PATH}" ]]; then
    echo "  LD_LIBRARY_PATH=/oem/usr/lib:/usr/lib ${REMOTE_DEST}"
fi
