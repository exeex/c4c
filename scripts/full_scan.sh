#!/bin/bash
# Full scan helper: run all registered c-testsuite tests in parallel via CTest.
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-build_debug}"
JOBS="${JOBS:-0}"
C_TESTSUITE_ROOT="${C_TESTSUITE_ROOT:-tests/c-testsuite}"

if [[ "$JOBS" -le 0 ]]; then
  if command -v sysctl >/dev/null 2>&1; then
    JOBS="$(sysctl -n hw.ncpu)"
  else
    JOBS=4
  fi
fi

mkdir -p build/agent_state
RAW_LOG=/tmp/full_scan_raw.txt

if [[ -d "$C_TESTSUITE_ROOT" ]]; then
  cmake -S . -B "$BUILD_DIR" -DC_TESTSUITE_ROOT="$C_TESTSUITE_ROOT" >/dev/null
else
  cmake -S . -B "$BUILD_DIR" >/dev/null
fi
cmake --build "$BUILD_DIR" >/dev/null

echo "[full_scan] build_dir=${BUILD_DIR} jobs=${JOBS}"
ctest --test-dir "$BUILD_DIR" -L c_testsuite -j "$JOBS" --output-on-failure \
  2>&1 | tee "$RAW_LOG"

echo "[full_scan] Done. Log: ${RAW_LOG}"
