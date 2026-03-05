# tiny-c2ll Plan (First-Fail Workflow Snapshot)

Last updated: 2026-03-05

## Current State

- Active frontend is C++ in `src/frontend_c/`.
- Python frontend has been removed.
- Core CTest suites are green in local baseline (`277/277`).
- `llvm_gcc_c_torture` is now run with guardrails:
  - per-step timeout
  - per-test timeout
  - optional runtime mem/cpu cap (`ulimit`, best effort)

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

## Current First Failure (as of 2026-03-05)

- Test: `llvm_gcc_c_torture_20000223_1_c`
- Failure: `[FRONTEND_TIMEOUT]` (20s) during frontend compile step.
- Priority: investigate and reduce pathological compile behavior for this case first.

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
ctest --test-dir build_debug --output-on-failure -R '^llvm_gcc_c_torture_20000223_1_c$' -j 1
```

## Notes for Handoff

- Timeout-related failures are not “soft” signals in this workflow; they are treated as blocking failures.
- Keep timeout/resource guards enabled while fixing, so regressions are caught early.
