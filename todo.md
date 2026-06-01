Status: Active
Source Idea Path: ideas/open/75_shared_aggregate_transport_plan_probe.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Shared Transport Authority If Required

# Current Packet

## Just Finished

Step 3 of `plan.md` implemented the first shared prepared aggregate-transport
authority slice without migrating AArch64 consumers.

- Added `PreparedAggregateTransportKind`,
  `PreparedAggregateTransportChunkKind`,
  `PreparedAggregateTransportScratchKind`,
  `PreparedAggregateTransportScratchRequirement`,
  `PreparedAggregateTransportChunk`, `PreparedAggregateTransportLane`, and
  `PreparedAggregateTransportPlan` adjacent to `PreparedCallArgumentPlan`.
- Added `PreparedCallArgumentPlan::aggregate_transport`.
- Published a conservative `byval_register_lanes` aggregate transport plan from
  `call_plans.cpp` only when the prepared source selection is already
  `ByvalRegisterLane` and required source, destination, size, alignment, lane,
  and occupied-register facts are present.
- Added prepared-printer visibility for aggregate transport kind, payload/copy
  size and alignment, source offsets, chunk facts, lane facts, destination
  register facts, occupied registers, and abstract scratch requirement.
- Added a focused prepared call-plan contract test for an AArch64 byval
  register-lane aggregate argument, including prepared dump visibility.
- No AArch64 lowering/printer consumer migration occurred in this packet.

## Suggested Next

Next coherent Step 3 packet: migrate the smallest AArch64 byval register-lane
aggregate consumer path to read `PreparedCallArgumentPlan::aggregate_transport`
for chunk/lane/source facts, keeping opcode choice, concrete scratch register
identity, address spelling, target records, and final printer formatting
AArch64-local.

## Watchouts

- The new contract currently publishes only the already-identified AArch64
  `ByvalRegisterLane` prepared source-selection case.
- Missing required facts intentionally leave `aggregate_transport` absent
  rather than re-deriving or guessing target-local details.
- Keep variadic aggregate `va_arg` plans separate.
- Do not combine the next migration with general memory cleanup, i128/f128
  carrier cleanup, or unrelated helper consolidation.
- Do not weaken aggregate testcase expectations or add named-case shortcuts.

## Proof

Command:

```bash
cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_prealloc_call_boundary_classification)$' >> test_after.log
```

Result: passed, 3/3 selected tests. Proof log: `test_after.log`.
