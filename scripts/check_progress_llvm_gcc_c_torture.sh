#!/bin/bash
# Iterative progress check for llvm gcc-c-torture:
# - defaults to stop on first failure (fast fix loop)
# - optionally rewrites allowlist to keep only failed cases
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-build_debug}"
LLVM_TEST_SUITE_ROOT="${LLVM_TEST_SUITE_ROOT:-tests/external/gcc_torture}"
LABEL="${LABEL:-llvm_gcc_c_torture}"
STOP_ON_FAILURE="${STOP_ON_FAILURE:-1}"
JOBS="${JOBS:-1}"
PRUNE_FAILED_ALLOWLIST="${PRUNE_FAILED_ALLOWLIST:-1}"

STEP_TIMEOUT_SEC="${STEP_TIMEOUT_SEC:-20}"
TEST_TIMEOUT_SEC="${TEST_TIMEOUT_SEC:-90}"
RUN_MEM_MB="${RUN_MEM_MB:-1024}"
RUN_CPU_SEC="${RUN_CPU_SEC:-20}"

mkdir -p build/agent_state
RAW_LOG=/tmp/check_progress_llvm_gcc_c_torture_raw.txt

if [[ "$STOP_ON_FAILURE" == "1" && "$JOBS" -ne 1 ]]; then
  echo "[check_progress_llvm] STOP_ON_FAILURE=1 requires JOBS=1. Forcing JOBS=1."
  JOBS=1
fi

if [[ -d "$LLVM_TEST_SUITE_ROOT" ]]; then
  cmake -S . -B "$BUILD_DIR" \
    -DLLVM_TEST_SUITE_ROOT="$LLVM_TEST_SUITE_ROOT" \
    -DLLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC="$STEP_TIMEOUT_SEC" \
    -DLLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC="$TEST_TIMEOUT_SEC" \
    -DLLVM_GCC_C_TORTURE_RUN_MEM_MB="$RUN_MEM_MB" \
    -DLLVM_GCC_C_TORTURE_RUN_CPU_SEC="$RUN_CPU_SEC" >/dev/null
else
  cmake -S . -B "$BUILD_DIR" >/dev/null
fi
cmake --build "$BUILD_DIR" >/dev/null

echo "[check_progress_llvm] build_dir=${BUILD_DIR} label=${LABEL} jobs=${JOBS} stop_on_failure=${STOP_ON_FAILURE}"
CTEST_ARGS=(--test-dir "$BUILD_DIR" -L "$LABEL" -j "$JOBS" --output-on-failure)
if [[ "$STOP_ON_FAILURE" == "1" ]]; then
  CTEST_ARGS+=(--stop-on-failure)
fi

set +e
ctest "${CTEST_ARGS[@]}" --output-log "$RAW_LOG"
CTEST_RC=$?
set -e

grep -E "Start|Passed|Failed|100%|Total Test time" "$RAW_LOG" || true

grep -E "Total Test time|tests passed|tests failed" "$RAW_LOG" > build/agent_state/last_result.txt || true

if [[ "$PRUNE_FAILED_ALLOWLIST" == "1" ]]; then
  set +e
  cmake --build "$BUILD_DIR" --target prune_llvm_gcc_c_torture_allowlist_to_failed
  PRUNE_RC=$?
  set -e
  if [[ "$PRUNE_RC" -ne 0 ]]; then
    echo "[check_progress_llvm] prune target failed; keeping current allowlist."
  fi
fi

if [[ "$CTEST_RC" -eq 0 ]]; then
  echo "[check_progress_llvm] All selected tests passed."
else
  echo "[check_progress_llvm] Tests failed (ctest rc=${CTEST_RC}). See ${RAW_LOG}."
fi
echo "[check_progress_llvm] Summary written to build/agent_state/last_result.txt"
