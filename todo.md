Status: Active
Source Idea Path: ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Address Text and Prepared Frame-Slot Operand Helpers

# Current Packet

## Just Finished

Step 2 extracted the AArch64 frame-slot address text, fixed-slot base policy,
prepared frame-slot lookup wrappers, prepared local load offset/address helpers,
and prepared frame-slot `MemoryOperand` construction into
`src/backend/mir/aarch64/codegen/frame_slot_address.hpp` and
`src/backend/mir/aarch64/codegen/frame_slot_address.cpp`.

The new source is listed in `src/backend/CMakeLists.txt`. The moved declarations
no longer live in `memory.hpp`, and the moved definitions no longer live in
`memory.cpp`. Direct AArch64 consumers now include `frame_slot_address.hpp`
explicitly where they use the extracted helpers. `find_frame_slot` and
`find_stack_object` remain as AArch64-local wrapper spellings, but now forward
to `prepare::find_frame_slot_by_id` and `prepare::find_stack_object_by_id`
instead of carrying independent scan loops.

Kept out of this step per packet scope: `register_indirect_address`,
`memory_address`, `materialize_frame_slot_memory_address_lines`, f128 transport
ownership, variadic `va_list` field addressing, scratch selection, opcode
selection, and generic memory operand ownership.

## Suggested Next

Delegate Step 3, `Extract Frame-Slot Address Materialization Lines`, as the next
ordered packet. Move `materialize_frame_slot_memory_address_lines` into the
frame-slot owner while preserving scratch choice and opcode-selection
boundaries; `f128.cpp` is the main direct touchpoint because it currently uses
both the extracted prepared operand constructor and the still-memory-owned
frame-slot materializer.

## Watchouts

- `materialize_frame_slot_memory_address_lines` intentionally remains in
  `memory.cpp`/`memory.hpp`; it still depends on scratch materialization and was
  explicitly out of scope for Step 2.
- `f128.cpp` includes both `frame_slot_address.hpp` and `memory.hpp` because it
  uses the extracted prepared operand constructor and the still-memory-owned
  frame-slot materializer.
- `comparison.cpp` still has a branch-fusion-specific local
  `find_frame_slot` wrapper, but it now forwards to `prepare::find_frame_slot_by_id`.
- `alu.cpp` and `cast_ops.cpp` still contain differently named local frame-slot
  lookup/base helpers outside the exact Step 2 target; do not treat this packet
  as having generalized every local frame-slot helper in AArch64 codegen.

## Proof

Ran the supervisor-selected proof exactly:

```sh
cmake --build build --target backend_aarch64_memory_operand_records_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_memory_operand_contract_test backend_prepared_lookup_helper_test c4cll && ctest --test-dir build -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|backend_codegen_route_aarch64_(variadic_aggregate_overflow_byte_copy|dynamic_stack_fixed_slot_uses_fp_anchor)|backend_prepared_lookup_helper)$' --output-on-failure > test_after.log
```

Result: build passed and CTest passed 6/6. Proof log: `test_after.log`.
