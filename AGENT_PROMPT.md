# Agent Task: tiny-c2ll First-Fail Repair Loop (2026-03-05)

## Mission

You are taking over the C++ frontend in `src/frontend_c/` and should improve correctness with small, verifiable fixes.

Primary goal now:
- Drive `llvm_gcc_c_torture` by **first failure only** and fix iteratively.

## Current Baseline

- Active compiler frontend is C++ only: `src/frontend_c/`.
- Stage1 binary: `build_debug/tiny-c2ll-stage1`.
- Core suites are green in latest local baseline (`277/277`).
- New torture workflow is enabled:
  - `scripts/check_progress_llvm_gcc_c_torture.sh`
  - `STOP_ON_FAILURE=1` (default)
  - `FRONTEND_TIMEOUT` is treated as a test failure by CTest.

## First Failing Torture Case Snapshot

Current first failing case in iterative run:
- `llvm_gcc_c_torture_20000223_1_c`
- Failure kind: `[FRONTEND_TIMEOUT]` at 20s in `run_llvm_gcc_c_torture_case.cmake`

Interpretation rule:
- Timeout is a real fail for this workflow (likely pathological compile behavior / excessive work).

## First Steps (every iteration)

1. Read `plan.md`.
2. Run `git status --short`.
3. Run first-fail loop:
   - `./scripts/check_progress_llvm_gcc_c_torture.sh`
4. Fix exactly one smallest root cause slice in frontend/parsing/IR path.

## Work Loop

1. Reproduce first failing case (from script output/log).
2. Implement one focused change.
3. Re-run:
   - `./scripts/check_progress_llvm_gcc_c_torture.sh`
4. If the old first-fail passes, continue to the new first-fail.
5. Record notes in `build/agent_state/progress_log.md`.
6. Commit with precise scope.

## Hard Rules

- Do not edit vendored `tests/c-testsuite/` or vendored `tests/llvm-test-suite/`.
- Keep changes minimal and test-backed.
- If blocked >15 minutes, write blocker + hypothesis in `build/agent_state/hard_bugs.md` and switch slice.
- Do not disable timeout guards to “pass” tests.

## Suggested Commands

```bash
# Configure/build
cmake -S . -B build_debug
cmake --build build_debug -j8

# First-fail torture loop (default: stop on first fail, prune allowlist to failed)
./scripts/check_progress_llvm_gcc_c_torture.sh

# If you want to inspect without modifying allowlist:
PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh

# Focused single-case run (example)
ctest --test-dir build_debug --output-on-failure -R '^llvm_gcc_c_torture_20000223_1_c$' -j 1
```

## Exit Criteria Per Change

- First failing case is fixed or failure mode is narrowed with concrete evidence.
- No unrelated regressions introduced in touched slice.
- Clear focused commit message and short note in progress log.
