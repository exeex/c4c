#!/bin/bash
# Full scan: run all c-testsuite cases and find newly passing ones.
# Compiler selection:
#   COMPILER_MODE=python (default) -> src/frontend/c2ll.py
#   COMPILER_MODE=cxx              -> ./build_debug/tiny-c2ll-stage1
# You can override directly with COMPILER=/path/to/compiler.
set -euo pipefail

cd "$(dirname "$0")/.."

WORKDIR="${WORKDIR:-/tmp/c4agent_fullscan}"
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
  echo "[full_scan] Missing C++ compiler binary: $COMPILER"
  echo "[full_scan] Build it first: cmake -S . -B ${BUILD_DIR} && cmake --build ${BUILD_DIR}"
  exit 1
fi

mkdir -p build/agent_state

RAW_LOG=/tmp/full_scan_raw.txt
PASS_LOG=/tmp/full_scan_passing.txt

echo "[full_scan] mode=${COMPILER_MODE} compiler=${COMPILER} workdir=${WORKDIR}"
echo "[full_scan] Running full testsuite scan..."
python3.14 tests/run_c_testsuite.py \
  --compiler "$COMPILER" \
  --clang "$CLANG_BIN" \
  --testsuite-root tests/c-testsuite \
  --workdir "$WORKDIR" 2>&1 | tee "$RAW_LOG" | grep SUMMARY || true

# Extract passing tests
grep "^\[PASS\]" "$RAW_LOG" | awk '{print $2}' | sort > "$PASS_LOG" || true

# Find newly passing (not yet in allowlist)
comm -23 "$PASS_LOG" <(sort tests/c_testsuite_allowlist.txt) > build/agent_state/newly_passing.txt || true

NEWLY=$(wc -l < build/agent_state/newly_passing.txt | tr -d ' ')
echo "[full_scan] Newly passing tests not in allowlist: $NEWLY"
if [[ "$NEWLY" -gt 0 ]]; then
  echo "[full_scan] See build/agent_state/newly_passing.txt"
  cat build/agent_state/newly_passing.txt
fi
