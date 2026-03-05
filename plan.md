# tiny-c2ll Plan (First-Fail Workflow Snapshot)

Last updated: 2026-03-06

## Current State

- Active frontend is C++ in `src/frontend_c/`.
- Python frontend has been removed.
- Core CTest suites are green in local baseline (`277/277`).
- `llvm_gcc_c_torture` is now run with guardrails:
  - per-step timeout
  - per-test timeout
  - optional runtime mem/cpu cap (`ulimit`, best effort)

## Latest Validation Snapshot (2026-03-06)

Checked incoming Claude changes in:
- `src/frontend_c/ast.hpp`
- `src/frontend_c/token.hpp`
- `src/frontend_c/token.cpp`
- `src/frontend_c/parser.cpp`
- `tests/llvm_gcc_c_torture_allowlist.txt`

Observed status:
- Build: pass (`cmake --build build_debug -j8`)
- Core smoke tests: pass
  - `tiny_c2ll_tests`
  - `frontend_cxx_preprocessor_tests`
- Current torture allowlist has been pruned to one case:
  - `20010605-2.c`
- Current first fail:
  - `[FRONTEND_FAIL] parse error: expected RPAREN but got 'x' at line 21`
  - case: `llvm_gcc_c_torture_20010605_2_c`

## Active Repair Strategy

Use **first-fail iterative repair** for `llvm_gcc_c_torture`:

```bash
./scripts/check_progress_llvm_gcc_c_torture.sh
```

Default behavior:
- `STOP_ON_FAILURE=1`: stop at first failed case.
- `PRUNE_FAILED_ALLOWLIST=1`: rewrite allowlist to keep failed cases only.
- `FRONTEND_TIMEOUT` is treated as a real test failure.

This is the intended workflow for continuous agent-driven fixing.

## Current First Failure (as of 2026-03-06)

- Test: `llvm_gcc_c_torture_20010605_2_c`
- Failure: `[FRONTEND_FAIL] parse error (expected RPAREN but got 'x')`.
- Priority: parser robustness fix for this case first.

## Execution Rules

1. One smallest fix slice per commit.
2. Re-run first-fail loop after each slice.
3. Do not edit vendored test-suite sources:
   - `tests/c-testsuite/`
   - `tests/llvm-test-suite/`
4. If blocked >15 min, log blocker in `build/agent_state/hard_bugs.md` and move to next hypothesis.

## Useful Commands

```bash
# Build
cmake -S . -B build_debug
cmake --build build_debug -j8

# First-fail loop (recommended)
./scripts/check_progress_llvm_gcc_c_torture.sh

# Inspect without rewriting allowlist
PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh

# Single case repro (example)
ctest --test-dir build_debug --output-on-failure -R '^llvm_gcc_c_torture_20010605_2_c$' -j 1
```

## Notes for Handoff

- Timeout-related failures are not “soft” signals in this workflow; they are treated as blocking failures.
- Keep timeout/resource guards enabled while fixing, so regressions are caught early.
