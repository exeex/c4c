#!/bin/bash
# Quick progress check: run c-testsuite allowlist tests via CTest.
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
RAW_LOG=/tmp/check_progress_raw.txt

if [[ -d "$C_TESTSUITE_ROOT" ]]; then
  cmake -S . -B "$BUILD_DIR" -DC_TESTSUITE_ROOT="$C_TESTSUITE_ROOT" >/dev/null
else
  cmake -S . -B "$BUILD_DIR" >/dev/null
fi
cmake --build "$BUILD_DIR" >/dev/null

echo "[check_progress] build_dir=${BUILD_DIR} jobs=${JOBS}"
ctest --test-dir "$BUILD_DIR" -L c_testsuite -j "$JOBS" --output-on-failure \
  2>&1 | tee "$RAW_LOG" | grep -E "Start|Passed|Failed|100%|Total Test time" || true

grep -E "Total Test time|tests passed|tests failed" "$RAW_LOG" > build/agent_state/last_result.txt || true
echo "[check_progress] Done. See build/agent_state/last_result.txt"
