# Agent Prompt: tiny-c2ll Stabilization Playbook

Last updated: 2026-03-07

## Mission

Improve correctness of `src/frontend_c/` with **small, verified, reversible** fixes.
Primary acceptance metric is full-suite pass rate from `ctest -j`.

## Non-Negotiable Working Style

1. Never guess ABI behavior when a failing case can be compared against clang IR.
2. For codegen/runtime mismatch, always produce a clang `.ll` baseline first.
3. Fix one root cause at a time; prove with targeted test, then broader regression check.
4. Do not hide failures by editing vendored tests under `tests/c-testsuite/` or `tests/llvm-test-suite/`.
5. If a test is temporarily allowlisted/disabled, record exact reason + unblock plan in `plan.md`.

## Required Debug Flow (for frontend/backend failures)

Given a failing case `<case>.c`:

1. Reproduce with ctest:
   - `ctest --output-on-failure -R <test_name>`
2. Capture our IR:
   - `build/tiny-c2ll-stage1 <case>.c -o /tmp/ours.ll`
3. Capture clang IR on same target triple:
   - `clang -target aarch64-unknown-linux-gnu -S -emit-llvm -O0 <case>.c -o /tmp/clang.ll`
4. Diff only the failing function/path:
   - call signature (especially variadic)
   - aggregate coercion
   - va_list layout/offset math
   - load/store alignment and memcpy size
5. Patch smallest mismatch.
6. Re-run:
   - failing test
   - nearby bucket tests (same subsystem)
   - `ctest -j` before handoff

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

## Suggested Command Set

```bash
# configure/build
cmake -S . -B build
cmake --build build -j8

# targeted repro
ctest --test-dir build --output-on-failure -R <test_name>

# full regression gate
ctest --test-dir build -j

# clang IR baseline for ABI issues
clang -target aarch64-unknown-linux-gnu -S -emit-llvm -O0 <case>.c -o /tmp/clang.ll
```

## Collaboration Notes

- Prefer explicit hypotheses: "I think mismatch is in call-site coercion".
- Show evidence (line snippets / function-level diffs), then patch.
- Avoid broad refactors during active failure loops.
