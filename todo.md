Status: Active
Source Idea Path: ideas/open/565_prepared_move_bundle_widening_stack_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Widening Authority Coverage

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

Recommended next packet: teach RV64 object-route consumption to lower
`PreparedMoveAuthorityKind::StackSlotWideningConversion` for stack-slot source
to stack-slot destination integer widening without treating it as a raw
same-width byte copy. Keep the repair constrained to the explicit authority
shape, preserve generic rejection for authority-none memory-to-memory copies,
and then run the two representative allowlist rows to classify any downstream
owner after RV64 materializes the widening move.

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
