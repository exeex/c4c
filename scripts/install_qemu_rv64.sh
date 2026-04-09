#!/usr/bin/env bash
set -euo pipefail

PACKAGES=(
  qemu-user
  qemu-system-misc
  binutils-riscv64-linux-gnu
  libc6-dev-riscv64-cross
  linux-libc-dev-riscv64-cross
  libstdc++-14-dev-riscv64-cross
)

log() {
  printf '[install_qemu_rv64] %s\n' "$*"
}

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "error: missing required command: $1" >&2
    exit 1
  fi
}

need_cmd apt-get

if [[ "${EUID}" -ne 0 ]]; then
  SUDO=(sudo)
else
  SUDO=()
fi

log "Updating apt package index"
"${SUDO[@]}" apt-get update

log "Installing minimal RV64 QEMU/sysroot packages"
"${SUDO[@]}" apt-get install -y "${PACKAGES[@]}"

log "Installed packages:"
printf '  %s\n' "${PACKAGES[@]}"
