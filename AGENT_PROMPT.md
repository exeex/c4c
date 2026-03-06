# Agent Task: tiny-c2ll Frontend-Failure Loop (2026-03-06)

## Mission

You are taking over the C++ frontend in `src/frontend_c/` and should improve correctness with small, verifiable fixes.

Primary goal now:
- Solve actionable frontend/compiler issues from `llvm_gcc_c_torture`.

## Current Baseline

- Active compiler frontend: `src/frontend_c/`.
- Stage1 binary: `build_debug/tiny-c2ll-stage1`.
- Core smoke tests are green.
- `20010605-2.c` is fixed (GNU `__real__/__imag__` support).
- `tests/llvm_gcc_c_torture_allowlist.txt` now targets entries from:
  - `tests/llvm_gcc_c_torture_frontend_failures.tsv`

## Current First Failure (Focused Allowlist)

- Test: `llvm_gcc_c_torture_20010122_1_c`
- Failure kind: `[BACKEND_FAIL]`
- Symptom: linker undefined symbols on arm64 (`___builtin_return_address`, `_alloca`)
- Interpretation: non-frontend/platform-runtime issue; skip/xfail for frontend-fix loop.

## First Steps (every iteration)

1. Read `plan.md`.
2. Run `git status --short`.
3. Run first-fail loop without auto-pruning:
   - `PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh`
4. If fail is non-frontend (`CLANG_COMPILE_FAIL` / platform `BACKEND_FAIL`), comment it out in allowlist and rerun.
5. Fix one smallest actionable frontend slice.

## Work Loop

1. Reproduce first fail from script output.
2. Classify fail:
   - `FRONTEND_FAIL` / `FRONTEND_TIMEOUT`: actionable frontend.
   - `CLANG_COMPILE_FAIL` / platform-link `BACKEND_FAIL`: non-frontend, skip in allowlist.
3. Implement one focused fix (or one skip-entry update).
4. Rerun first-fail loop.
5. Record notes in `build/agent_state/progress_log.md`.
6. Commit with precise scope.

## Hard Rules

- Do not edit vendored `tests/c-testsuite/` or `tests/llvm-test-suite/`.
- Keep changes minimal and test-backed.
- If blocked >15 minutes, write blocker + hypothesis in `build/agent_state/hard_bugs.md` and switch slice.
- Do not disable timeout/resource guards to force pass.

## Suggested Commands

```bash
# sandbox build path for agent:
export AGENT_NAME=claude

cmake -S . -B build_${AGENT_NAME}
cmake --build build_${AGENT_NAME} -j8

# First-fail loop for focused allowlist
PRUNE_FAILED_ALLOWLIST=0 ./scripts/check_progress_llvm_gcc_c_torture.sh

# Single-case repro
ctest --test-dir build_${AGENT_NAME} --output-on-failure -R '^llvm_gcc_c_torture_20010122_1_c$' -j 1
```

## Exit Criteria Per Change

- First failing actionable frontend case is fixed, or
- One non-frontend blocker case is cleanly classified/skipped in allowlist.
- No unrelated regressions introduced in touched slice.
