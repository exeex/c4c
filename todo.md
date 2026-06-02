Status: Active
Source Idea Path: ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract Frame-Slot Address Materialization Lines

# Current Packet

## Just Finished

Step 3 moved `materialize_frame_slot_memory_address_lines` out of
`src/backend/mir/aarch64/codegen/memory.hpp` and
`src/backend/mir/aarch64/codegen/memory.cpp` into the existing frame-slot
address owner,
`src/backend/mir/aarch64/codegen/frame_slot_address.hpp` and
`src/backend/mir/aarch64/codegen/frame_slot_address.cpp`.

The extracted helper preserves the existing non-frame-slot no-op, immediate
`add`/`sub` form for offsets up to 4095, large-offset
`materialize_integer_constant_lines` route, negative-offset `sub` behavior,
base selection from `uses_frame_pointer_base`, and caller-owned scratch
selection. `f128.cpp` continues to call the helper through its existing
`frame_slot_address.hpp` include; generic `memory_address`, f128 transport
ownership, variadic `va_list` field addressing, and register-indirect formatting
were not moved.

## Suggested Next

Delegate Step 4, `Check Memory-Backed f128 and Variadic Consumers`, as the next
ordered packet. Audit the memory-backed f128 and variadic consumers touched or
affected by the extraction while keeping f128 transport policy, ABI logic,
scratch selection, storage-plan decisions, and `va_list` field addressing at
their current owners.

## Watchouts

- Step 3 intentionally did not move `memory_address`, generic memory operand
  ownership, register-indirect formatting, f128 transport ownership, variadic
  `va_list` field addressing, scratch choice, or opcode selection.
- `f128.cpp` still includes both `frame_slot_address.hpp` and `memory.hpp`: it
  uses the moved frame-slot materializer through the frame-slot owner and still
  uses generic memory helpers such as `memory_address`.
- Step 4 should verify nearby memory-backed f128 and variadic paths consume the
  same prepared frame-slot facts after the owner move, without treating this as
  permission to absorb ABI or transport responsibilities into
  `frame_slot_address.*`.

## Proof

Ran the supervisor-selected Step 3 proof exactly:

```sh
cmake --build build --target backend_aarch64_memory_operand_records_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|machine_printer|instruction_dispatch))$' --output-on-failure > test_after.log
```

Result: build passed and CTest passed 4/4. Proof log: `test_after.log`.
