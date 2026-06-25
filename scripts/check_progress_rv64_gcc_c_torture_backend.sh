#!/bin/bash
# Progress scan for gcc-c-torture through the RV64 BIR/MIR object path:
#   C source -> c4cll --codegen obj -> clang link -> qemu-riscv64
# The result is compared against clang's RV64 binary under the same qemu/sysroot.
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-build}"
LLVM_TEST_SUITE_ROOT="${LLVM_TEST_SUITE_ROOT:-tests/c/external/gcc_torture}"
ALLOWLIST="${ALLOWLIST:-$LLVM_TEST_SUITE_ROOT/allowlist.txt}"
RUNNER="${RUNNER:-tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake}"
TARGET_TRIPLE="${TARGET_TRIPLE:-riscv64-linux-gnu}"
SYSROOT="${SYSROOT:-/usr/riscv64-linux-gnu}"
CASE_TIMEOUT_SEC="${CASE_TIMEOUT_SEC:-20}"
STOP_ON_FAILURE="${STOP_ON_FAILURE:-0}"
MAX_CASES="${MAX_CASES:-0}"
VERBOSE_FAILURES="${VERBOSE_FAILURES:-0}"

C4CLL="${C4CLL:-$BUILD_DIR/c4cll}"
CLANG="${CLANG:-$(command -v clang || true)}"
QEMU_RISCV64="${QEMU_RISCV64:-$(command -v qemu-riscv64 || true)}"

BUILD_DIR="$(realpath -m "$BUILD_DIR")"
LLVM_TEST_SUITE_ROOT="$(realpath -m "$LLVM_TEST_SUITE_ROOT")"
ALLOWLIST="$(realpath -m "$ALLOWLIST")"
RUNNER="$(realpath -m "$RUNNER")"
C4CLL="$(realpath -m "$C4CLL")"

if [[ ! -f "$ALLOWLIST" ]]; then
  echo "[rv64-gcc-torture] missing allowlist: $ALLOWLIST" >&2
  exit 2
fi
if [[ ! -f "$RUNNER" ]]; then
  echo "[rv64-gcc-torture] missing runner: $RUNNER" >&2
  exit 2
fi
if [[ -z "$CLANG" || ! -x "$CLANG" ]]; then
  echo "[rv64-gcc-torture] clang not found" >&2
  exit 2
fi
if [[ -z "$QEMU_RISCV64" || ! -x "$QEMU_RISCV64" ]]; then
  echo "[rv64-gcc-torture] qemu-riscv64 not found" >&2
  exit 2
fi
if [[ ! -d "$SYSROOT" ]]; then
  echo "[rv64-gcc-torture] missing RV64 sysroot: $SYSROOT" >&2
  exit 2
fi

cmake --build "$BUILD_DIR" --target c4cll >/dev/null
if [[ ! -x "$C4CLL" ]]; then
  echo "[rv64-gcc-torture] c4cll not found: $C4CLL" >&2
  exit 2
fi

STATE_DIR="$BUILD_DIR/agent_state"
WORK_ROOT="$BUILD_DIR/rv64_gcc_c_torture_backend"
SUMMARY="$STATE_DIR/rv64_gcc_c_torture_backend_summary.tsv"
FAILED="$STATE_DIR/rv64_gcc_c_torture_backend_failed.txt"
mkdir -p "$STATE_DIR" "$WORK_ROOT"
printf "# status\tcase\tlog\n" >"$SUMMARY"
: >"$FAILED"

total=0
passed=0
failed=0

while IFS= read -r raw_line || [[ -n "$raw_line" ]]; do
  entry="${raw_line%%#*}"
  entry="${entry%%|*}"
  entry="$(printf '%s' "$entry" | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')"
  [[ -z "$entry" ]] && continue

  total=$((total + 1))
  if [[ "$MAX_CASES" -gt 0 && "$total" -gt "$MAX_CASES" ]]; then
    total=$((total - 1))
    break
  fi

  src="$LLVM_TEST_SUITE_ROOT/$entry"
  case_id="$(printf '%s' "$entry" | sed 's#[^A-Za-z0-9_.-]#_#g')"
  case_dir="$WORK_ROOT/$case_id"
  log="$case_dir/case.log"
  mkdir -p "$case_dir"
  rm -f "$case_dir"/*.bin "$case_dir"/*.o "$log"

  if [[ ! -f "$src" ]]; then
    failed=$((failed + 1))
    printf "missing\t%s\t%s\n" "$entry" "$log" | tee -a "$SUMMARY"
    printf "%s\n" "$entry" >>"$FAILED"
    [[ "$STOP_ON_FAILURE" == "1" ]] && break
    continue
  fi

  printf "[RUN][rv64-gcc-torture-backend] %s\n" "$entry"
  set +e
  cmake \
    -DCOMPILER="$C4CLL" \
    -DCLANG="$CLANG" \
    -DQEMU_RISCV64="$QEMU_RISCV64" \
    -DSRC="$src" \
    -DROOT="$LLVM_TEST_SUITE_ROOT" \
    -DTARGET_TRIPLE="$TARGET_TRIPLE" \
    -DSYSROOT="$SYSROOT" \
    -DOUT_CLANG_BIN="$case_dir/clang.bin" \
    -DOUT_OBJECT="$case_dir/c4c.o" \
    -DOUT_C4C_BIN="$case_dir/c4c.bin" \
    -DCASE_TIMEOUT_SEC="$CASE_TIMEOUT_SEC" \
    -P "$RUNNER" >"$log" 2>&1
  rc=$?
  set -e

  if [[ "$rc" -eq 0 ]]; then
    passed=$((passed + 1))
    printf "pass\t%s\t%s\n" "$entry" "$log" | tee -a "$SUMMARY"
  else
    failed=$((failed + 1))
    printf "fail\t%s\t%s\n" "$entry" "$log" | tee -a "$SUMMARY"
    printf "%s\n" "$entry" >>"$FAILED"
    if [[ "$VERBOSE_FAILURES" == "1" ]]; then
      tail -n 20 "$log" || true
    fi
    [[ "$STOP_ON_FAILURE" == "1" ]] && break
  fi
done <"$ALLOWLIST"

printf "[rv64-gcc-torture] total=%d passed=%d failed=%d\n" "$total" "$passed" "$failed"
printf "[rv64-gcc-torture] summary=%s failed=%s work=%s\n" "$SUMMARY" "$FAILED" "$WORK_ROOT"

if [[ "$failed" -ne 0 ]]; then
  exit 1
fi
