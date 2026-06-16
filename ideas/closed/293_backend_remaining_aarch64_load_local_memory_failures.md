# Backend Remaining AArch64 Load Local-Memory Failures

## Goal

Triage and repair the remaining `^backend_` red tests after the 286-292
prepared/interface cleanup sequence.

The current backend scope is down to three AArch64 route failures, all blocked
before assembly emission by semantic BIR `load local-memory` admission.

## Why This Exists

After idea 291 closed and the reopened 286/288 blocker was repaired in idea
292, the focused 286/288/291 subsets are green. A fresh backend sweep still
reports 177/180 passing, with three failures:

- `backend_codegen_route_aarch64_byval_payload_8_to_13_stack_overflow`
- `backend_codegen_route_aarch64_byval_payload_9_to_14_stack_overflow`
- `backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value`

The two byval stack-overflow payload cases fail in function `same_bytes`; the
pointer-value writeback case fails in `main`. All three fail in the
`load local-memory semantic family`.

## In Scope

- Reproduce the exact three-test subset and capture the first rejected LIR/BIR
  shape for each failure family.
- Decide whether the byval stack-overflow payload failures and pointer-value
  writeback failure share one semantic memory admission gap or require separate
  follow-up ideas.
- Repair the smallest general semantic BIR local-memory admission rule that
  covers one coherent failure family.
- Preserve the completed 286/288/291 behavior:
  - no rendered `alignstack(...)` recovery as structured authority;
  - no prepared-only or printed-BIR bypass;
  - no weakening of opaque compatibility gates from ideas 289/290.
- Add focused backend proof for any newly admitted load shape.

## Out of Scope

- Broad AArch64 ABI or prepared call-plan rewrites.
- Retiring public prepared lookup surfaces.
- Updating expected assembly snippets without fixing semantic BIR admission.
- Named-case shortcuts for `same_bytes`, `main`, `byval_helper_payload_*`, or
  `aarch64_pointer_value_named_scalar_writeback`.
- Claiming full backend health until the complete `^backend_` scope is green.

## Acceptance Criteria

- The closure note records a failure-family classification for all three
  current red tests.
- At least one coherent failure family is repaired semantically, or the idea
  generates separate follow-up ideas with precise owners and proof commands.
- Focused tests prove the repaired load-local shape without relying on the
  named failing fixture.
- The selected three-test subset improves monotonically and does not introduce
  new backend failures.
- A close or milestone proof runs:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`
  and records the remaining pass/fail count.

## Closure Note

Closed after all three original AArch64 backend failures were classified as
one semantic local-memory admission boundary for loads through opaque runtime
pointer values:

- `backend_codegen_route_aarch64_byval_payload_8_to_13_stack_overflow`
- `backend_codegen_route_aarch64_byval_payload_9_to_14_stack_overflow`
- `backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value`

The repair admitted the intended pointer-value local-memory load/store shapes
without fixture, function, payload-size, or target-specific matching. Focused
BIR proof was added for runtime pointer-value opaque `i32` access, while the
existing fail-closed notes coverage for unsupported casted byte-pointer `i32`
opaque compatibility remained green.

Close proof recorded in `todo.md` before archival:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
`180/180` backend tests with `0` failures, including all three original red
tests. The supervisor-provided full-suite hook baseline candidate for commit
`73543a60` was accepted at `3208/3208` passing. The close-time regression guard
checker reported PASS on canonical `test_before.log` and `test_after.log`.

## Reviewer Reject Signals

- The patch changes required/forbidden snippets, CTest labels, or expected
  assembly while semantic BIR still fails in the same load-local family.
- The patch recognizes named fixtures, functions, or payload sizes instead of
  lowering a general local-memory shape.
- The patch bypasses semantic BIR through prepared-only, printed-BIR, rendered
  call-argument text, or target-specific acceptance.
- The patch weakens fail-closed behavior introduced by ideas 289-292.
- The closure claims all backend failures are solved without a fresh
  `^backend_` proof.
