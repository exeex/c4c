# Stack-Layout Test Surface Cleanup After Backend Ownership Migration

Status: Open
Last Updated: 2026-04-09

## Goal

Trim the remaining stack-layout test helpers that still rehydrate broad
`StackLayoutInput` views now that production ownership has moved onto prepared
backend-CFG-backed rewrite/planning inputs.

## Why This Idea Exists

The Step 8 closeout audit for idea 42 found that the remaining public helpers
under `src/backend/stack_layout/` are no longer hidden ownership shims:

- production entrypoints use prepared backend-owned liveness, planning, and
  rewrite packets
- the surviving `StackLayoutInput` rehydration helpers are used by tests that
  still compare new prepared carriers against the old broad surface
- deleting those helpers now would be broad test-surface cleanup rather than
  required backend-ownership migration work

That cleanup is worth doing, but it should not keep idea 42 open once the
ownership migration itself is complete.

## Scope

### In Scope

- audit remaining `StackLayoutInput`-rehydration helpers that are only used in
  tests
- retarget parity tests to narrower prepared/planning/rewrite helpers where
  practical
- delete public compatibility helpers once equivalent focused assertions exist
- tighten helper comments so test-only compatibility surfaces are obvious

### Out of Scope

- reopening production regalloc, liveness, or stack-layout ownership
- changing backend behavior unrelated to helper-surface cleanup
- bundling unrelated backend API cleanup just because tests touch nearby files

## Candidate Targets

- `src/backend/stack_layout/slot_assignment.hpp`
- `src/backend/stack_layout/slot_assignment.cpp`
- `tests/backend/backend_shared_util_tests.cpp`

## Completion Check

- remaining public stack-layout helpers describe either production ownership or
  explicit test-only compatibility
- tests no longer depend on broad helper wrappers when the narrower prepared
  seam already exposes the same facts
