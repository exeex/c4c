Status: Active
Source Idea Path: ideas/open/242_prepared_stack_slot_preserved_value_extent.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Prepared Extent Fields

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding optional prepared extent fields for
stack-slot `PreparedCallPreservedValue` records and feeding them from
prepared/shared authority only.

Concrete Step 2 results:

- Added `stack_size_bytes` and `stack_align_bytes` to
  `PreparedCallPreservedValue`.
- Published assignment extents through `PreparedStackSlotAssignment` and
  `PreparedValueHome` so value-home and assigned-stack-slot preservation
  routes copy prepared facts rather than recomputing type extents.
- Populated the `value.stack_object_id` branch directly from
  `PreparedFrameSlot::size_bytes` and `PreparedFrameSlot::align_bytes`.
- Tightened stack-slot route completeness to require slot id, stack offset,
  size, alignment, and spill placement. Register-route completeness remains
  unchanged.

## Suggested Next

Start `plan.md` Step 3 by exposing stack-slot preserved-value size and
alignment in prepared observation output, with focused coverage selected by
the supervisor.

## Watchouts

- Prepared extents now exist in memory but are not yet printed by prepared
  observation output; Step 3 owns that visibility.
- AArch64 lowering was intentionally untouched. Future consumers should use
  the prepared fields rather than reconstructing extents locally.
- Stack-slot completeness now rejects zero or absent size/alignment; if a
  later route creates stack-slot preserved values without assignment or frame
  extents, it should surface as an incomplete route instead of fabricated
  target-local data.

## Proof

Ran the delegated proof:

`cmake --build --preset default > test_after.log 2>&1`

`ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: build passed and all `139/139` selected backend tests passed.
Canonical proof log: `test_after.log`.
