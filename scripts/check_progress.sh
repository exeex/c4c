#!/bin/bash
# Quick progress check: run allowlist only, print summary.
# Compiler selection:
#   COMPILER_MODE=python (default) -> src/frontend/c2ll.py
#   COMPILER_MODE=cxx              -> ./build_debug/tiny-c2ll-stage1
# You can override directly with COMPILER=/path/to/compiler.
set -euo pipefail

cd "$(dirname "$0")/.."

WORKDIR="${WORKDIR:-/tmp/c4agent_check}"
BUILD_DIR="${BUILD_DIR:-build_debug}"
CLANG_BIN="${CLANG_BIN:-/usr/bin/clang}"
COMPILER_MODE="${COMPILER_MODE:-python}"
COMPILER="${COMPILER:-}"

if [[ -z "$COMPILER" ]]; then
  if [[ "$COMPILER_MODE" == "cxx" ]]; then
    COMPILER="./${BUILD_DIR}/tiny-c2ll-stage1"
  else
    COMPILER="src/frontend/c2ll.py"
  fi
fi

if [[ "$COMPILER_MODE" == "cxx" && ! -x "$COMPILER" ]]; then
  echo "[check_progress] Missing C++ compiler binary: $COMPILER"
  echo "[check_progress] Build it first: cmake -S . -B ${BUILD_DIR} && cmake --build ${BUILD_DIR}"
  exit 1
fi

mkdir -p build/agent_state
RAW_LOG=/tmp/check_progress_raw.txt

echo "[check_progress] mode=${COMPILER_MODE} compiler=${COMPILER} workdir=${WORKDIR}"
python3.14 tests/run_c_testsuite.py \
  --compiler "$COMPILER" \
  --clang "$CLANG_BIN" \
  --testsuite-root tests/c-testsuite \
  --workdir "$WORKDIR" \
  --allowlist tests/c_testsuite_allowlist.txt 2>&1 | tee "$RAW_LOG" | grep -E "SUMMARY|FAIL|PASS" || true

grep SUMMARY "$RAW_LOG" > build/agent_state/last_result.txt || true
echo "[check_progress] Done. See build/agent_state/last_result.txt"
