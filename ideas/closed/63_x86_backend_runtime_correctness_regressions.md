# X86 Backend Runtime Correctness Regressions

Status: Closed
Created: 2026-04-20
Last-Updated: 2026-04-20
Closed: 2026-04-20
Disposition: Completed; the known runtime-owned x86 backend cases now execute correctly and closure validation showed monotonic improvement with no new grouped-scope regressions.
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Why This Was Closed

Idea 63 existed to keep already-emitting x86 backend cases in a runtime
correctness bucket instead of hiding them inside unsupported-capability
diagnostics. That owned route is now satisfied for the known failure family:

- `c_testsuite_x86_backend_src_00040_c` now executes correctly after the
  same-module prepared runtime seam repair
- `c_testsuite_x86_backend_src_00086_c` is green in the current tree
- `c_testsuite_x86_backend_src_00130_c` now executes correctly after the
  overlapping prepared frame-slot normalization fix

The active runbook is therefore exhausted because the source idea's completion
signal is met: the owned runtime-failure cases execute correctly under the
backend c-testsuite route.

## Validation At Closure

Closure used the following grouped runtime scope:

- `cmake --build --preset default`
- `ctest --test-dir build --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_(00040|00086|00130)_c)$' | tee test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result:

- grouped proof passed with `77` passed / `0` failed / `77` total
- regression guard passed monotonically against the pre-fix grouped baseline
- before reported `75` passed / `2` failed / `77` total
- after reported `77` passed / `0` failed / `77` total
- resolved failing tests:
  - `c_testsuite_x86_backend_src_00040_c`
  - `c_testsuite_x86_backend_src_00130_c`
- no new grouped-scope failing tests were introduced

## Intent

Track backend c-testsuite cases that already cross compilation and codegen
gates but still fail at runtime or produce the wrong result.

This leaf exists so correctness bugs are not hidden inside unsupported-capacity
buckets.

## Owned Failure Family

This idea owns current runtime correctness failures from the 2026-04-20 backend
run, including crashes and wrong-result executions.

## Current Known Failed Cases It Owns

- `c_testsuite_x86_backend_src_00040_c`
  Current observed failure mode: segmentation fault after advancing past the
  old prepared call-bundle rejection path.
- `c_testsuite_x86_backend_src_00086_c`
  Current observed failure mode: segmentation fault.
- `c_testsuite_x86_backend_src_00130_c`
  Current observed failure mode: runtime nonzero / wrong result.

## Scope Notes

Expected repair themes include:

- diagnosing whether the root cause is bad lowering, bad addressing, bad call
  setup, or bad emit logic
- preserving the distinction between "unsupported" and "incorrect"
- adding proof that nearby already-running cases do not regress

## Boundaries

This idea does not own broad unsupported-capability families that still abort
before runtime. Those belong in ideas 58 through 62.

## Dependencies

This idea can be executed independently for bug repair, but its root causes may
lead back into other leaves. If so, keep the correctness ownership here and
cross-reference the enabling capability idea rather than collapsing the two.

## Completion Signal

This idea is complete when the owned runtime-failure cases execute correctly
under the backend c-testsuite route.
