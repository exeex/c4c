Status: Active
Source Idea Path: ideas/open/242_prepared_stack_slot_preserved_value_extent.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove AArch64 Can Consume Prepared Extents

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by making the AArch64 preserved-value consumer
convert complete prepared stack-slot records into structured memory-preserve
effects.

Concrete Step 4 results:

- AArch64 call preserve lowering now emits a `MachineEffectResourceKind::Memory`
  preserve effect for stack-slot preserved values only when prepared slot id,
  stack offset, size, and alignment are present and nonzero.
- The memory preserve operand is built from the prepared slot id, prepared stack
  offset, prepared size, and prepared alignment; it does not infer extents from
  value type, slot ordering, object names, or frame-slot lookup.
- AArch64 live-preservation completeness now requires stack-slot preserved-value
  size and alignment before machine-printer helper boundaries accept the route.
- Focused AArch64 target-record and dispatch tests now prove the prepared
  stack-slot extent fields become a structured memory-preserve effect.
- Focused AArch64 target-record tests now also prove stack-slot preserved
  values with slot/offset but missing prepared size or missing prepared
  alignment fail closed without producing a memory preserve effect.

## Suggested Next

Supervisor should review Step 4 for acceptance and choose the next lifecycle
packet, likely Step 5 validation/reviewer readiness if no broader AArch64
integration is required.

## Watchouts

- Stack-slot preserved values without prepared size or alignment still fail
  closed and produce no memory preserve effect.
- The memory effect uses the prepared stack offset directly as the operand byte
  offset. No target-local frame-slot lookup or size/alignment reconstruction was
  added.

## Proof

Ran the delegated proof:

`cmake --build --preset default > test_after.log 2>&1`

`ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: build passed and all `139/139` selected backend tests passed.
Canonical proof log: `test_after.log`.
