Status: Active
Source Idea Path: ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Supported Aggregate va_arg Helper Shape

# Current Packet

## Just Finished

Step 3 completed the contract-publication prerequisite for RV64 overflow-area
aggregate `va_arg` lowering without implementing object-route aggregate helper
lowering.

- Added structured `payload_write_address` data to
  `PreparedVariadicAggregateVaArgAccessPlan`, carrying the destination pointer
  result value plus exact prepared frame-slot address materialization block,
  instruction, slot, and byte offset.
- Populated the fact only from exact prepared address-materialization authority
  for the aggregate destination pointer at the helper call site.
- Printed the fact as `payload_write_address=%value/frame_slot:slot=#...` with
  materialization block/instruction details.
- Updated the focused RV64 dump contract to assert the distinct case where
  `destination_payload=%t1/stack_slot` remains stack slot `#37` offset `48`
  while `payload_write_address=%t1/frame_slot` is frame slot `#28` offset `35`.
- Kept RV64 object aggregate `va_arg` lowering fail-closed when the structured
  write-address fact is absent, while complete current aggregate helper facts
  still reject as unsupported lowering rather than guessing.

## Suggested Next

Proceed with the next supervisor-selected packet for RV64 object-route
aggregate `va_arg` lowering by consuming `payload_write_address` directly from
the prepared aggregate access plan.

## Watchouts

- RV64 object emission still deliberately rejects complete aggregate
  `va_arg_aggregate` helper plans with
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower
  va_arg_aggregate helper`.
- Do not infer the payload write object from raw BIR names, address-selection
  side channels, or joins between aggregate value names and addressing records.
- `destination_payload_home` remains present for existing consumers, but the
  concrete write-address fact for the next RV64 lowering packet is the
  structured `payload_write_address`.
- When exact prepared address materialization cannot be found, the RV64 planner
  records `helper_operand_homes.va_arg_aggregate.payload_write_address` as the
  missing fact and the RV64 object route rejects the helper as incomplete.
- Existing root-level `test_before.log` and baseline logs were left in place;
  this packet wrote the delegated proof output to `test_after.log`.

## Proof

Delegated proof passed and was written to `test_after.log`:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- CTest reported `100% tests passed, 0 tests failed out of 326`.
