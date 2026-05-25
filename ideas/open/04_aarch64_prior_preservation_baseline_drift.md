# AArch64 Prior Preservation Baseline Drift

## Goal

Investigate and repair the full-suite baseline drift that first appears at
`11b33c8d0586` (`Retire prior-preservation selected fallback`).

## Why This Exists

`log/baseline_08a7f725a3f5820506a517f58ae9b9012bc20b7e.log` is clean with
`100% tests passed, 0 tests failed out of 3410`. The next relevant drift point
is `log/baseline_11b33c8d0586b44d163de6b74bff9c33957aab0b.log`, which reports
`37 tests failed out of 3410`, all in `aarch64_backend c_testsuite`.

The triggering commit only changes `calls_moves.cpp`, one dispatch test, and
`todo.md`. The failure family therefore points at the removal of a
prior-preservation selected fallback in AArch64 call-boundary move lowering.

## In Scope

- Compare the clean baseline `08a7f725a3f5` with the drifting baseline
  `11b33c8d0586`.
- Inspect `src/backend/mir/aarch64/codegen/calls_moves.cpp` prior-preservation
  call argument source selection.
- Reproduce or narrow at least one failing `c_testsuite_aarch64_backend_*`
  case.
- Restore semantic call argument preservation without reviving broad
  testcase-shaped fallback behavior.
- Keep tests focused on the missing prior-preservation source-selection rule.

## Out Of Scope

- Broad AArch64 call-plan architecture cleanup.
- Reverting unrelated later commits.
- Downgrading c_testsuite expectations.
- Treating the 37 failures as unrelated independent cases.

## Acceptance Criteria

- The first bad baseline family is explained with a concrete missing lowering
  rule.
- The AArch64 backend no longer regresses the representative failing tests.
- Focused proof covers the repaired prior-preservation/source-selection path.
- No unsupported markings, expectation weakening, or named-test shortcuts are
  used.

## Reviewer Reject Signals

- A patch simply restores the old broad fallback and reintroduces rederived
  prior homes for incomplete explicit selections.
- A patch special-cases the listed c_testsuite names.
- A patch weakens baseline or c_testsuite contracts.
- A patch claims the drift is fixed without proving at least one failing case
  and the focused call-boundary path.
