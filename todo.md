Status: Active
Source Idea Path: ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Producer Classification Or Rejection

# Current Packet

## Just Finished

Step 3 - Repair Producer Classification Or Rejection repaired the bankless
prepared frame-slot producer for known scalar stack-slot homes.

`src/backend/prealloc/storage_plans.cpp` now lets storage-plan publication
recover a stack-slot value type from the prepared BIR function when the
matching regalloc record is absent or only publishes `type=void` /
`register_class=none`. That BIR/home authority is used in prealloc only; RV64
object emission still does not infer GPR bank from scalar type after the
storage plan has been published.

The repair is intentionally stack-slot scoped. Empty regalloc class records no
longer erase a BIR-derived scalar bank for stack-slot homes, but non-stack
homes such as immediate F128 constants keep the existing `bank=none` behavior.
Focused prealloc coverage now constructs the `%t8`-like case directly: a
stack-slot home with slot/offset facts, a BIR `i16` result, and a same-name
regalloc record with no usable type or class. The resulting storage plan must
publish `encoding=frame_slot`, `bank=gpr`, width 1, slot id, stack offset, and
structured spill-slot placement.

Focused `pr69447.c` proof now shows `%t8` as `storage %t8 frame_slot
bank=gpr` in both focused and full storage-plan dump lines. Object emission
still fails closed for the remaining `%t8 i16 -> %t9 i64` stack-slot move with
no conversion authority; no raw stack-copy materialization was added.

## Suggested Next

Proceed to Step 4 by adding or refining the owner-specific RV64 rejection for
conversion-adjacent stack-slot movement with mismatched scalar types and no
explicit conversion materialization contract.

## Watchouts

The `pr69447.c` object route still reports
`fragment_status=generic_move_bundle_materialization_failed` after the bank
repair, but the diagnostic now includes `source_type=i16` and
`destination_type=i64`. Step 4 should make that conversion-adjacent rejection
owner-specific without accepting the raw stack copy and without implementing
conversion materialization.

## Proof

Proof is captured in `test_after.log`:

- `cmake --build --preset default`
- Focused `pr69447.c` prepared dump for `%t8`, showing `bank=gpr`.
- Focused `pr69447.c` object-emission probe, expected nonzero, still failing
  closed with `unsupported_move_bundle_target_shape`, `reason=consumer_stack_to_stack`,
  `source_type=i16`, and `destination_type=i64`.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `git diff --check -- src/backend/prealloc/storage_plans.cpp tests/backend/bir/backend_prealloc_decoded_home_storage_test.cpp todo.md`
