# tiny-c2ll Execution Plan

Last updated: 2026-03-07

## Objective

Raise full-suite stability by fixing real root causes in parser/typing/IR lowering, with strict regression control.

## Current Baseline

- Full run command of record: `ctest -j`
- Recent status: `89% tests passed, 184 failed out of 1735`
- Recent successful high-value fix pattern:
  - Reproduce fail
  - Compare against clang `.ll`
  - Patch smallest ABI/IR mismatch
  - Re-verify targeted + full suite

## Priority Order

1. P0: `negative_tests` compile-fail contracts (must not regress)
2. P1: `c_testsuite` runtime/ABI correctness
3. P2: `llvm_gcc_c_torture` conformance expansion

## Mandatory Loop (each repair slice)

1. Select one failing test and classify: parser / semantic / ABI / runtime.
2. Reproduce with exact `ctest -R` command.
3. For ABI/runtime/codegen failures, generate both IRs:
   - ours: `tiny-c2ll-stage1 case.c -o /tmp/ours.ll`
   - clang: `clang -target aarch64-unknown-linux-gnu -S -emit-llvm -O0 case.c -o /tmp/clang.ll`
4. Identify one concrete mismatch and patch only that slice.
5. Run:
   - the failing test
   - sibling tests in same bucket
   - `ctest -j` (acceptance gate)
6. Log result in this file (short entry with date, case, root cause, status).

## Hard Constraints

1. No edits to vendored upstream tests under:
   - `tests/c-testsuite/`
   - `tests/llvm-test-suite/`
2. No "fix" that only masks failures (blanket skips/allowlists) without documented rollback plan.
3. No merge if new regressions appear in previously passing tests.

## Evidence Template (append per slice)

Use this exact format:

```md
### YYYY-MM-DD: <test_name>
- Symptom:
- Root cause:
- Clang IR delta used: yes/no
- Files changed:
- Targeted re-run:
- Full `ctest -j` delta:
- Residual risk:
```

## Next Slice Queue

1. Continue `c_testsuite` ABI-heavy failures first (va_arg/aggregate/HFA class).
2. Then parser/semantic clusters with highest fan-out.
3. Revisit allowlist/temporary skips only after root-cause fixes are exhausted.

## Exit Criteria

1. Full-suite pass rate increases without new P0 regressions.
2. High-risk ABI path has clang-aligned behavior on representative cases.
3. Handoff notes include exact commands and proof points, not just conclusions.
