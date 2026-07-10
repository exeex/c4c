Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Narrow Stack-Carried Pointer Rule

# Current Packet

## Just Finished

Step 3, `Implement The Narrow Stack-Carried Pointer Rule`: added the
producer-side prepared authority carrier for stack-carried pointer values whose
source is an explicit `PointerBasePlusOffset` local-frame address
materialization and whose selected preservation home is a complete stack slot.

- Changed files:
  `src/backend/prealloc/calls.hpp`,
  `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/prepared_printer/calls.cpp`, and
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`.
- `PreparedCallPreservedValue` now has an optional `source_selection` fact.
  Call-preservation planning populates it only when exactly one call-argument
  source selection proves the same value id/name, `PointerBasePlusOffset`
  source home, local-frame materialization fields, and complete preserved
  stack home.
- Prior-preservation source selection copies that explicit source identity
  while preserving the selected stack home in `preserved_*` fields. Missing,
  stale, mismatched, stack-slot-only, and ambiguous producer states remain
  fail-closed because the selection predicate rejects them or leaves
  `source_selection` absent.
- Focused unit coverage checks the positive `%t6`-class shape and negative
  missing-materialization, stale-value, and stack-slot-only shapes. The check
  runs before the existing known FPR dump failure in
  `backend_prepare_frame_stack_call_contract`.

## Suggested Next

Executor packet: consume the explicit stack-carried pointer source authority in
the RV64 prior-preservation stack-slot path for the `%t6` family. Require the
new source-selection fact plus the preserved stack home to match before
materializing or reloading the branch operand; keep stack-slot-only
preservation rejected for pointer-source materialization.

## Watchouts

- Do not reopen RV64 terminator-fragment admission from idea 645.
- Do not infer pointer freshness or materialization from stack offsets, final
  assembly shape, source spelling, local names, diagnostics, testcase identity,
  runtime outcomes, or pass/fail accounting.
- `loop-2e.c` now passes the direct runtime runner; do not use `%t23` as the
  first failing runtime proof unless a later packet identifies a still-red
  focused owner.
- The producer authority is now explicit, but this packet did not change the
  RV64 object-emission consumer. RV64 still must not infer from stack offsets,
  source spelling, final assembly shape, or testcase identity.
- Stack-slot-only preservation remains valid as ordinary stack preservation;
  it is not a pointer source materialization authority.
- `backend_prepare_frame_stack_call_contract` still exits later at the known
  `rv64 FPR ABI/frame fact contract` dump assertion, so use the new check's
  placement before that assertion when evaluating focused coverage.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365. The failed-test count and list match the known
`test_before.log` backend subset shape; no new backend failure set was
introduced. `test_after.log` is the preserved proof log.

Additional focused proof: `cmake --build --preset default --target
backend_prepare_frame_stack_call_contract_test &&
build/tests/backend/bir/backend_prepare_frame_stack_call_contract_test` builds
and reaches the existing `rv64 FPR ABI/frame fact contract` failure after the
new stack-carried pointer source authority check has run.
