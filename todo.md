Status: Active
Source Idea Path: ideas/open/242_prepared_stack_slot_preserved_value_extent.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Expose Extents In Prepared Observations

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by exposing optional stack-slot preserved-value
extent fields in prepared observation output and focused prepared-printer
coverage.

Concrete Step 3 results:

- Prepared callsite summaries now append `:size=N:align=N` for stack-slot
  `PreparedCallPreservedValue` records when those optional fields are present.
- Detailed prepared call-plan preserve lines now print `stack_size=N` and
  `stack_align=N` when present.
- `backend_prepared_printer` now requires the stack-crossing stack-slot
  preservation fixture to publish and print both prepared extent fields.

## Suggested Next

Supervisor should review Step 3 for acceptance and choose the next lifecycle
packet.

## Watchouts

- Prepared observation output now exposes the in-memory stack-slot preserved
  extents without recomputing type sizes in the printer.
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
