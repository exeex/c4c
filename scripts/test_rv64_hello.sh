#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="${WORK_DIR:-/tmp/c4c-rv64-hello}"
SRC="$WORK_DIR/hello.cc"
BIN="$WORK_DIR/hello-rv64"
SYSROOT="${SYSROOT:-/usr/riscv64-linux-gnu}"
CLANGXX="${CLANGXX:-clang++}"
QEMU_RISCV64="${QEMU_RISCV64:-qemu-riscv64}"

log() {
  printf '[test_rv64_hello] %s\n' "$*"
}

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "error: missing required command: $1" >&2
    exit 1
  fi
}

need_cmd "$CLANGXX"
need_cmd "$QEMU_RISCV64"

mkdir -p "$WORK_DIR"

cat > "$SRC" <<'EOF'
#include <iostream>

int main() {
  std::cout << "hello rv64\n";
  return 0;
}
EOF

log "Compiling hello world with clang for riscv64-linux-gnu"
"$CLANGXX" \
  --target=riscv64-linux-gnu \
  --gcc-toolchain=/usr \
  "$SRC" \
  -o "$BIN"

log "Running hello world under qemu-riscv64"
"$QEMU_RISCV64" -L "$SYSROOT" "$BIN"

log "Done: $BIN"
