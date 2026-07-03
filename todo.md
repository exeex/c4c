Status: Active
Source Idea Path: ideas/open/565_prepared_move_bundle_widening_stack_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Prepared Widening Authority

# Current Packet

## Just Finished

Completed plan Step 2, `Add Focused Widening Authority Coverage`, for
`ideas/open/565_prepared_move_bundle_widening_stack_authority.md`.

Changed files:

- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Added a minimal prepared contract surface,
`PreparedMoveAuthorityKind::StackSlotWideningConversion`, and taught
`src/backend/prealloc/regalloc.cpp::normalize_prepared_move_publication(...)`
to publish it only for consumer stack-slot to stack-slot moves whose prepared
source home byte width is smaller than the destination home byte width. This
keeps same-width stack-to-stack copies and unsupported move-bundle shapes on
their existing paths while making conversion-adjacent widening authority
explicit before RV64 object-route consumption.

Added focused backend coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` for both
representative width families, `i8 -> i32` and `i16 -> i32`. The test asserts
that the prepared authority is visible as
`stack_slot_widening_conversion` and that RV64 still fail-closes instead of
lowering the widening as a plain stack-to-stack byte copy.

Supervisor representative probe:

- `build/agent_state/565_step2_widening_authority_after.allowlist`
- `build/agent_state/565_step2_widening_authority_after.log`

Result: expected representative failures remain, `total=2 passed=0 failed=2`,
but both rows moved off the old
`unsupported_prepared_move_bundle_classification` prepared classifier
rejection. Both now fail as `unsupported_move_bundle_target_shape` with
`authority=stack_slot_widening_conversion` and
`fragment_status=generic_move_bundle_materialization_failed`.

Representative facts after this slice:

- `src/20010224-1.c`: stack-slot source `i16`, stack-slot destination `i32`,
  explicit prepared authority `stack_slot_widening_conversion`.
- `src/pr87623.c`: stack-slot source `i8`, stack-slot destination `i32`,
  explicit prepared authority `stack_slot_widening_conversion`.

Step 3 remains, but it should be re-scoped to consume the new explicit
prepared authority in RV64 or split it into an equivalent prepared conversion
plus stack-destination lowering. The producer-side authority publication is
already partially consumed by this Step 2 slice, and the current owner has
moved to RV64 consumption/materialization of that explicit prepared authority.

## Suggested Next

Plan Step 3, `Repair Prepared Widening Authority`, is ready for an executor
packet.

Executor packet:

- Objective: teach RV64 object-route consumption/materialization to lower
  `PreparedMoveAuthorityKind::StackSlotWideningConversion` semantically for
  stack-slot source to stack-slot destination integer widening.
- Expected lowering: consume or split the explicit prepared authority into an
  integer value extension from the narrower source slot plus a stack
  destination write. Do not authorize it as a raw stack byte-copy.
- Owned boundaries: RV64 move-bundle/object-route materialization for the
  explicit `stack_slot_widening_conversion` authority and directly adjacent
  tests. Prepared authority publication from Step 2 is already complete.
- Required preservation: authority-none memory-to-memory copies and unrelated
  unsupported move-bundle shapes must still reject/fail closed.
- Representative checks after backend proof: run the two-row allowlist for
  `src/20010224-1.c` and `src/pr87623.c` with verbose failures, record whether
  either row passes or moves to a downstream owner, and keep any distinct
  residual work out of this step unless it is required to materialize the
  widening authority.
- Proof command: use the supervisor-selected backend subset command and write
  the canonical executor proof to `test_after.log`.
- Done when: focused backend tests pass, the backend subset passes, RV64 no
  longer fails these rows at `unsupported_move_bundle_target_shape` for
  `authority=stack_slot_widening_conversion`, and `todo.md` records changed
  files, proof commands, representative results, and any downstream residual
  owner.

## Watchouts

- The new authority is semantic and width-driven; do not special-case
  representative filenames, event names, or diagnostic strings.
- A plain stack-to-stack byte-copy authorization remains semantically wrong for
  `i8/i16 -> i32`; RV64 consumption must extend or split the value, not copy
  four bytes from the narrower source slot.
- Do not touch expectations, unsupported markers, allowlists, or pass/fail
  accounting.
- Preserve fail-closed behavior for unrelated memory-to-memory copies and
  unsupported move-bundle shapes.
- The first delegated proof attempt hit an unrelated `cc1plus` killed build
  during a large AArch64 test compile. Rerunning the exact proof command
  incrementally reached CTest; after fixing the owned RV64 fixture metadata,
  the same exact proof command passed.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: passed, `345/345` backend tests. Canonical proof log:
`test_after.log`.
