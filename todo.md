Status: Active
Source Idea Path: ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Construction Through the Explicit Shape

# Current Packet

## Just Finished

Step 3 routed aggregate register-lane construction in `lower_before_call_move`
through the explicit `aggregate_register_lane_publication_view(move_record)`
boundary after each register-lane record sets
`move_record.move.reason = "call_arg_byval_aggregate_register_lanes"`.

The construction authority remains in `calls.cpp`: prepared-source selection,
destination register selection, ABI ownership, scratch/preservation decisions,
and ordinary call-boundary move construction are unchanged. The new local
validator only fail-closes aggregate register-lane records using the existing
prepared source/destination diagnostic style when the explicit view rejects the
constructed shape.

## Suggested Next

Step 4 should continue the schema-cleanup route from the now-explicit
construction boundary, likely by removing remaining duplicate aggregate-lane
shape checks or call-boundary record field peeking where the active plan calls
for it. Keep this scoped to aggregate-lane record consumption and avoid moving
ABI construction or prepared authority out of `calls.cpp`.

## Watchouts

- The construction validator deliberately reuses the existing aggregate
  register-lane missing source/destination diagnostic; do not broaden it into
  ordinary move validation.
- Printer-side validation is still needed after the view succeeds because
  scratch availability, occupied-lane destination selection, memory addressing,
  and chunk printability can still reject a selected shape.
- Step 4 should preserve the same ownership boundaries: no ABI construction
  movement, no expectation rewrites, and no named-case shortcuts.

## Proof

Step 3 proof passed and wrote CTest output to `test_after.log`.

```sh
cmake --build build --target backend_aarch64_call_boundary_owner_test backend_prepare_frame_stack_call_contract_test backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test c4cll && ctest --test-dir build -R '^(backend_aarch64_(call_boundary_owner|target_instruction_records|machine_printer|instruction_dispatch)|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address)$' --output-on-failure > test_after.log
```

CTest subset result: 6/6 passed.
