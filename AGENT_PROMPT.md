# Agent Prompt: tiny-c2ll Stabilization Playbook

Last updated: 2026-03-09

## Mission

Improve correctness of the frontend/codegen pipeline (`src/frontend/`, `src/codegen/llvm/`) with **small, verified, reversible** fixes.
Primary acceptance metric is full-suite pass rate from `ctest -j`.

## Non-Negotiable Working Style

1. Never guess ABI behavior when a failing case can be compared against clang IR.
2. For codegen/runtime mismatch, always produce a clang `.ll` baseline first.
3. Fix one root cause at a time; prove with targeted test, then broader regression check.
4. Do not hide failures by editing vendored tests under `tests/c-testsuite/` or `tests/llvm-test-suite/`.
5. If a test is temporarily allowlisted/disabled, record exact reason + unblock plan in `plan.md`.
6. If a patch breaks core functionality tests (especially `c_testsuite` cases), inspect and compare corresponding logic in `ref/claudes-c-compiler/src/frontend/` before further patching; prefer adopting a proven handling pattern that avoids fixing A while breaking B.

## Standard Workflow

1. Build and run baseline tests.
2. Pick one failing case to fix with this priority order:
   - `FRONTEND_FAIL` > `BACKEND_FAIL` > `RUNTIME_FAIL`
3. Patch one root cause, then rerun targeted and related tests.
4. Run full regression before submit.

Test timeout policy:
- Default per-test timeout: `30s`
- Any test runtime `>30s` is treated as suspicious regression unless explicitly documented.

## Required Debug Flow (Frontend/Backend Failures)

Given a failing case `<case>.c`:

1. Reproduce with ctest:
   - `ctest --test-dir build --output-on-failure -R <test_name>`
2. Capture our IR:
   - `build/tiny-c2ll-next <case>.c -o /tmp/ours.ll`
3. Capture clang IR on same target triple:
   - `clang -target aarch64-unknown-linux-gnu -S -emit-llvm -O0 <case>.c -o /tmp/clang.ll`
4. Diff only failing function/path:
   - call signature (especially variadic)
   - aggregate coercion
   - va_list layout/offset math
   - load/store alignment and memcpy size
5. Patch smallest mismatch.
6. Re-run:
   - failing test
   - nearby bucket tests (same subsystem)
   - full `ctest -j` before handoff

## Rule: Variadic / ABI Cases

When touching `va_arg`, variadic calls, struct passing, or HFA/HVA:

1. Must compare against clang-emitted IR for the same input.
2. Must verify both call-site coercion and callee va_arg extraction.
3. Must verify at least one complex ABI regression case (`00204.c` class).
4. Must not merge if it fixes one ABI case by breaking `negative_tests` contract checks.

## Acceptance Gates Per Patch

A patch is acceptable only if all pass:

1. Target failing test passes.
2. Related bucket stays green (or improves).
3. `ctest -j` does not introduce new regressions in previously passing tests.
4. Any temporary skip/allowlist change is documented with owner and exit criteria.
5. Full-suite result is strictly monotonic: total passed tests must increase, and no previously passing test may flip to failing.

## Suggested Command Set

```bash
# configure/build (debug + torture suite)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_LLVM_GCC_C_TORTURE_TESTS=ON
cmake --build build -j8

# targeted repro
ctest --test-dir build --output-on-failure -R <test_name>

# full regression gate
ctest --test-dir build -j --output-on-failure > test_fail.log

# clang IR baseline for ABI issues
clang -target aarch64-unknown-linux-gnu -S -emit-llvm -O0 <case>.c -o /tmp/clang.ll
```

## Commit Rule

Only submit a commit when:

1. Existing passing tests are not broken.
2. Total pass count improves (or clearly fixes targeted blocker without regressions).
3. Commit message records what changed and why.
4. When the patch meets the acceptance gates (including strict full-suite monotonic improvement), commit immediately in the same work cycle; do not leave verified fixes uncommitted.

## Collaboration Notes

- Prefer explicit hypotheses, e.g. "mismatch is in call-site coercion".
- Show evidence (line snippets / function-level diffs), then patch.
- Avoid broad refactors during active failure loops.
