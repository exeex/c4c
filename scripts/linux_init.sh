#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build_linux}"
KERNEL_SRC="${KERNEL_SRC:-$ROOT_DIR/tests/linux}"
KERNEL_OUT="${KERNEL_OUT:-$ROOT_DIR/tests/linux-build}"
JOBS="${JOBS:-$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN || echo 8)}"

log() {
  printf '[linux_init] %s\n' "$*"
}

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "error: missing required command: $1" >&2
    exit 1
  fi
}

need_cmd git
need_cmd cmake
need_cmd make
need_cmd clang
need_cmd flex
need_cmd bison

cd "$ROOT_DIR"

log "Init linux submodule"
if git ls-files --stage -- tests/linux | awk '{print $1}' | grep -q '^160000$'; then
  git submodule update --init --recursive tests/linux
else
  log "tests/linux is not a git-tracked submodule in current checkout; skip submodule init"
fi

log "Build Ubuntu compiler binaries (stage1/stage2) in $BUILD_DIR"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j"$JOBS" --target frontend_cxx_stage1 frontend_cxx_stage2

log "Generate kernel tinyconfig with clang (O=$KERNEL_OUT)"
mkdir -p "$KERNEL_OUT"
make -C "$KERNEL_SRC" O="$KERNEL_OUT" CC=clang tinyconfig

# Some Linux versions do not expose CONFIG_EMBEDDED anymore.
if command -v rg >/dev/null 2>&1; then
  has_embedded_config() {
    rg -n "^config EMBEDDED$" "$KERNEL_SRC" -g 'Kconfig*' >/dev/null 2>&1
  }
else
  has_embedded_config() {
    find "$KERNEL_SRC" -type f -name 'Kconfig*' -exec grep -qE "^config EMBEDDED$" {} + >/dev/null 2>&1
  }
fi

if has_embedded_config; then
  if [[ -x "$KERNEL_SRC/scripts/config" ]]; then
    "$KERNEL_SRC/scripts/config" --file "$KERNEL_OUT/.config" -e EMBEDDED || true
    make -C "$KERNEL_SRC" O="$KERNEL_OUT" CC=clang olddefconfig
  fi
fi

log "Done"
cat <<EOF
Next build command:
make -C "$KERNEL_SRC" O="$KERNEL_OUT" CC="$BUILD_DIR/tiny-c2ll-stage2" -j"$JOBS"
EOF
