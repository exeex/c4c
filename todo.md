Status: Active
Source Idea Path: ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Tighten Surface and Validate Acceptance

# Current Packet

## Just Finished

Step 5 audited the final AArch64 frame-slot address owner surface and validated
acceptance, without implementation edits.

`src/backend/mir/aarch64/codegen/memory.hpp` no longer exposes frame-slot
lookup, prepared frame-slot operand construction, frame-slot address spelling,
fixed-slot base selection, or frame-slot address materialization helpers. Its
remaining `register_indirect_address` declaration is generic register-offset
formatting used by non-frame-slot memory consumers.

`src/backend/mir/aarch64/codegen/frame_slot_address.hpp` exposes the narrow
AArch64 memory-local owner surface for frame-slot address spelling, fixed-slot
frame-pointer base policy, prepared frame-slot/stack-object fact consumption,
prepared local/frame-slot access helpers, prepared frame-slot `MemoryOperand`
construction, and frame-slot address materialization lines. The audit found no
temporary public declaration or boundary leak requiring code edits.

`src/backend/CMakeLists.txt` registers
`mir/aarch64/codegen/frame_slot_address.cpp`. Direct consumers include memory
lowering, f128 transport, variadic lowering, dispatch/publication/value
materialization paths, calls/globals/cast/fp/ALU helpers, and edge-copy paths.
Generic memory formatting remains in `memory.*`; f128 transport policy and
variadic `va_list`/aggregate-copy responsibilities remain outside the owner.

## Suggested Next

Lifecycle close is ready for supervisor/plan-owner review if the supervisor
accepts this Step 5 audit and the passing proof in `test_after.log`.

## Watchouts

- `find_stack_object` remains public in the owner because memory lowering uses
  it to consume prepared stack-object facts next to frame-slot destinations; it
  does not move stack-object planning or source-authority decisions into the
  owner.
- `memory.hpp` still intentionally exposes generic memory helper declarations
  such as `memory_address`, selected memory record construction/lowering, and
  `register_indirect_address`.

## Proof

Ran the supervisor-selected Step 5 proof exactly:

```sh
cmake --build build --target backend_aarch64_memory_operand_records_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_memory_operand_contract_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test c4cll && ctest --test-dir build -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|machine_printer|instruction_dispatch)|backend_codegen_route_aarch64_(variadic_aggregate_overflow_byte_copy|dynamic_stack_fixed_slot_uses_fp_anchor)|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log
```

Result: build passed and CTest passed 8/8. Proof log: `test_after.log`.
