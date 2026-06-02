Status: Active
Source Idea Path: ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce the Narrow Aggregate-Lane Schema or Accessor

# Current Packet

## Just Finished

Step 2 introduced `AggregateRegisterLanePublicationView` as a narrow typed view
over the existing `CallBoundaryMoveInstructionRecord` fields without changing
the record layout or construction flow.

The aggregate-lane shape checks now live in
`aggregate_register_lane_publication_view(move)`. The existing
`is_aggregate_register_lane_publication(move)` predicate delegates to that view,
and `print_aggregate_register_lane_publication_lines` consumes the selected
source memory, destination register, and size through the view while preserving
printer-side unsupported returns for scratch selection, lane destination
validation, and printable chunk selection.

No ABI construction, prepared-source authority, source/destination selection,
scratch decisions, preservation routing, or `calls.cpp` logic moved in this
packet.

## Suggested Next

Step 3 should use the new aggregate-lane view where the plan calls for
record-schema cleanup or boundary assertions, while keeping construction
authority in `calls.cpp` and avoiding any expansion into ABI lowering or
expectation changes.

## Watchouts

- Keep the view behavior-preserving: it mirrors the previous predicate,
  including `BeforeCall`, call-argument ABI register destination, move reason,
  prepared non-address source memory, `1..16` byte size, GPR destination bank,
  expected X view, and GP physical register checks.
- Printer-side validation is still needed after the view succeeds because
  scratch availability, occupied-lane destination selection, memory addressing,
  and chunk printability can still reject a selected shape.
- Do not broaden Step 3 into a new aggregate-lane payload or into `calls.cpp`
  ownership changes unless the supervisor changes the route.

## Proof

Step 2 proof passed and wrote CTest output to `test_after.log`.

```sh
cmake --build build --target backend_aarch64_call_boundary_owner_test backend_prepare_frame_stack_call_contract_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test c4cll && ctest --test-dir build -R '^(backend_aarch64_(call_boundary_owner|target_instruction_records|machine_printer)|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address)$' --output-on-failure > test_after.log
```

CTest subset result: 5/5 passed.
