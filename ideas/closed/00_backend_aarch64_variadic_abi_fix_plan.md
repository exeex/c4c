# AArch64 Variadic ABI Fix Plan

## Goal

Fix the remaining AArch64 backend variadic ABI failures so the affected backend
internal tests pass again without regressing the already-stable non-variadic
calling convention paths.

Current failing tests:

- `backend_lir_aarch64_variadic_dpair_ir`
- `backend_lir_aarch64_variadic_float_array_ir`
- `backend_lir_aarch64_variadic_nested_float_array_ir`
- `backend_lir_aarch64_variadic_float4_ir`
- `backend_lir_aarch64_variadic_double4_ir`
- `backend_lir_aarch64_variadic_single_double_ir`
- `backend_lir_aarch64_variadic_single_float_ir`

## Why This Is A Separate Idea

These failures are tightly clustered around one backend mechanism family:

- AArch64 variadic lowering
- ABI classification of floating-point and aggregate arguments
- IR and/or asm emission for varargs consumption paths

That scope is narrow enough to deserve its own idea instead of being hidden
inside the broader backend roadmap or mixed into unrelated parser/frontend work.

## Likely Failure Surface

Based on the failing test names, the current bug cluster is likely around one or
more of these seams:

- classification of FP scalars passed through varargs
- classification of small FP aggregates such as pairs / fixed arrays / float4
- stack vs register placement once an argument crosses into variadic handling
- promotion / packing mismatch between the call side and `va_arg`-style readback
- disagreement between backend lowering and the expected AArch64 procedure call
  standard for variadic cases

The plan should assume the root cause may be shared across several of the seven
tests, not seven unrelated bugs.

## Scope

Do:

- reproduce the current failing backend tests directly and capture the shared
  failure pattern
- inspect the AArch64 variadic ABI implementation path before changing code
- reduce the bug family to the smallest classification mistake(s) possible
- fix one mechanism family at a time
- add or keep backend-owned regression coverage close to the failing shapes

Do not:

- broaden this into a full AArch64 ABI redesign
- mix non-variadic calling convention cleanup into the same slice
- rewrite unrelated backend code while chasing these failures
- quietly absorb x86-64 or rv64 variadic work into this idea

## Primary Targets

Likely implementation surfaces:

- `src/backend/aarch64/codegen/calls.cpp`
- `src/backend/aarch64/codegen/variadic.cpp`
- `src/backend/aarch64/codegen/returns.cpp`
- `src/backend/call_abi.cpp`
- backend variadic tests under `tests/`

The exact write set should be chosen only after reproducing the failures and
locating the common classification seam.

## Execution Shape

Recommended order:

1. re-run the seven failing tests and group them by shared symptom
2. inspect the current AArch64 variadic argument classification path
3. compare the failing shapes:
   - scalar float / double
   - pair / fixed array aggregates
   - nested float aggregates
4. identify whether the break is in:
   - call lowering
   - `va_arg` consumption lowering
   - both
5. land the smallest fix that removes the common failure family
6. re-run all seven failing tests together
7. only then expand to nearby backend regression checks if needed

## Validation Strategy

Minimum validation for the future implementation:

- all seven listed tests pass
- no newly failing AArch64 backend internal tests are introduced
- any new regression case added should isolate the exact ABI shape that was
  previously wrong

## Handoff Notes

This idea is intentionally execution-oriented but not yet activated into the
single active `plan.md`.

Whoever picks it up should first verify whether the seven failing tests still
form the complete failure cluster, then keep the implementation scoped to the
shared AArch64 variadic ABI mechanism rather than patching each case
independently.

## Completion

Status: Complete

Completed on: 2026-03-29

Result:

- The seven-test failure cluster was reproduced and confirmed as a shared IR
  mismatch around the AArch64 floating-point varargs join path.
- The active implementation seam lived in
  `src/codegen/lir/stmt_emitter.cpp`, where AArch64 homogeneous FP aggregate
  `va_arg` lowering used `vaarg.hfa.*` labels instead of the shared
  `vaarg.fp.*` join shape expected by the backend IR checks.
- The fix was kept to the smallest shared mechanism change: rename the HFA
  varargs control-flow labels to the existing FP varargs family so the emitted
  IR matches the AArch64 backend contract already exercised by the tests.

Validation:

- `ctest --test-dir build --output-on-failure -R 'backend_lir_aarch64_variadic_(dpair|float_array|nested_float_array|float4|double4|single_double|single_float)_ir'`
- `ctest --test-dir build --output-on-failure -R '^backend_lir_aarch64_variadic_.*_ir$'`
- `ctest --test-dir build -j --output-on-failure > test_fail_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_fail_before.log --after test_fail_after.log --allow-non-decreasing-passed`

Observed regression-guard result:

- before: `2294 passed / 1 failed / 2295 total`
- after: `2341 passed / 0 failed / 2341 total`
- newly failing tests: `0`

Leftover Issues:

- None recorded from this runbook slice.
