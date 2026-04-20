# X86 Backend Runtime Correctness Regressions

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

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
