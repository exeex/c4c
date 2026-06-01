Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Value-Home, Storage, And Frame-Offset Authority Into Memory

# Current Packet

## Just Finished

Completed the first executable packet for `plan.md` Step 3 in
`src/backend/mir/aarch64/codegen/memory.cpp`. Extracted
`make_prepared_pointer_value_base_register(...)` as the shared prepared
pointer-value base home/storage decoder, preserving the existing
`PreparedMemoryOperandRecordError` mapping while requiring:

- a prepared pointer value id and name,
- a matching prepared register home,
- a matching register storage-plan encoding, and
- successful AArch64 register conversion through the existing local register
  view/class helper.

`resolve_pointer_value_base_register(...)`,
`make_load_memory_instruction_record(...)`, and
`make_store_memory_instruction_record(...)` now all use the shared helper for
pointer-value base register decoding.

## Suggested Next

Next coherent Step 3 packet: extract the shared prepared result value
home/storage decoding for load results, keeping stack-publication scratch
register selection and load opcode/register view policy local to
`memory.cpp`.

## Watchouts

The new pointer-base helper intentionally preserves the prior missing-id/name
behavior by returning `MissingPointerValueHome`; changing that to
`MissingPointerValueName` would be a separate contract change. Keep Step 4
stack-source publication planning out of the next Step 3 packet. Do not move
AArch64 register view selection, scratch choice, opcode choice, or
`frame_slot_address(...)`/`register_indirect_address(...)` spelling into
prepared helpers.

## Proof

Passed. `test_after.log` contains the delegated proof output: build completed
and the focused CTest subset passed 13/13.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|dynamic_stack_fixed_slot_uses_fp_anchor|pointer_value_named_scalar_writeback_uses_computed_store_value)|cli_dump_prepared_bir_(is_prepared|exposes_contract_sections|local_arg_call_contract|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
