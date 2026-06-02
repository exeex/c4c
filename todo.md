Status: Active
Source Idea Path: ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Check Memory-Backed f128 and Variadic Consumers

# Current Packet

## Just Finished

Step 4 audited the memory-backed f128 and variadic consumers after the
frame-slot address owner extraction, without implementation edits.

`src/backend/mir/aarch64/codegen/f128.cpp` consumes
`make_prepared_frame_slot_memory_operand` and
`materialize_frame_slot_memory_address_lines` through
`frame_slot_address.hpp`. The audit found the f128 transport policy, ABI
validation, memory-backed carrier checks, q-register scratch choice, reserved
address scratch choice, and non-frame-slot `memory_address` formatting still
owned by `f128.cpp`/generic memory helpers.

`src/backend/mir/aarch64/codegen/variadic.cpp` consumes only
`fixed_slots_use_frame_pointer` from `frame_slot_address.hpp`. The audit found
`va_list` field discovery/addressing, cursor/progression updates, aggregate
field-copy lowering, stack-address materialization, and register-indirect
formatting still owned by the variadic lowering path.

## Suggested Next

Delegate Step 5 as the next ordered packet. Validate the broader owner boundary
or route closure criteria the supervisor selects, using the Step 4 audit as the
handoff that the touched f128 and variadic consumers do not need implementation
changes.

## Watchouts

- Do not move f128 transport policy, memory-backed carrier validation, ABI
  register selection, scratch choice, generic `memory_address`, variadic
  `va_list` field addressing, cursor/progression updates, aggregate copy
  lowering, or register-indirect formatting into `frame_slot_address.*`.
- `f128.cpp` intentionally still includes both `frame_slot_address.hpp` and
  `memory.hpp`: the former owns prepared frame-slot helpers and the latter owns
  generic memory operand formatting.
- `variadic.cpp` intentionally uses the new owner only for
  `fixed_slots_use_frame_pointer`; its local stack-address helper still
  materializes `sp`-relative helper addresses for variadic lowering.

## Proof

Ran the supervisor-selected Step 4 proof exactly:

```sh
cmake --build build --target backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test c4cll && ctest --test-dir build -R '^(backend_aarch64_(machine_printer|instruction_dispatch)|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy)$' --output-on-failure > test_after.log
```

Result: build passed and CTest passed 3/3. Proof log: `test_after.log`.
