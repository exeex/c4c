# tiny-c2ll Plan (Frontend-Failure Focus)

Last updated: 2026-03-06

## Current State

- Active frontend is C++ in `src/frontend_c/`.
- Python frontend is removed.
- Core smoke suites pass:
  - `tiny_c2ll_tests`
  - `frontend_cxx_preprocessor_tests`
- `20010605-2.c` frontend fix is completed (`__real__/__imag__` support).

## Allowlist Strategy

- `tests/llvm_gcc_c_torture_allowlist.txt` is generated from:
  - `tests/llvm_gcc_c_torture_frontend_failures.tsv`
- Purpose: focus Claude/Codex loop on known frontend failure set instead of full ~1.7k cases.

## Current First Failure (Focused Run)

Command used:

```bash
PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh
```

Result:
- First fail: `llvm_gcc_c_torture_20010122_1_c`
- Failure kind: `[BACKEND_FAIL]`
- Error: arm64 link undefined symbols (`___builtin_return_address`, `_alloca`)
- Classification: non-frontend / platform runtime mismatch

Action policy:
1. Skip/comment non-frontend blocker cases in allowlist.
2. Continue first-fail loop until next actionable frontend failure.

## Active Repair Workflow

1. Run first-fail loop:
   - `PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh`
2. If fail is non-frontend (`CLANG_COMPILE_FAIL`, platform-link `BACKEND_FAIL`):
   - comment out case in `tests/llvm_gcc_c_torture_allowlist.txt`
   - rerun.
3. If fail is frontend (`FRONTEND_FAIL`, `FRONTEND_TIMEOUT`):
   - fix smallest root cause in parser/typing/IR.
4. Commit one small slice each iteration.

## Execution Rules

1. One smallest fix slice per commit.
2. Re-run first-fail loop after each slice.
3. Do not edit vendored test-suite sources:
   - `tests/c-testsuite/`
   - `tests/llvm-test-suite/`
4. If blocked >15 min, log blocker in `build/agent_state/hard_bugs.md` and switch hypothesis.

## Useful Commands

```bash
cmake -S . -B build_debug
cmake --build build_debug -j8

PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh

ctest --test-dir build_debug --output-on-failure -R '^llvm_gcc_c_torture_20010122_1_c$' -j 1
```
