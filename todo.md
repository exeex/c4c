Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Value-Home, Storage, And Frame-Offset Authority Into Memory

# Current Packet

## Just Finished

Completed the next executable packet for `plan.md` Step 3 in
`src/backend/mir/aarch64/codegen/memory.cpp`. Extracted
`decode_prepared_load_result_value_storage(...)` as the shared prepared load
result value home/storage decoder used by
`make_load_memory_instruction_record(...)`, preserving the existing
`PreparedMemoryOperandRecordError` mapping while centralizing:

- required result value id/name checks,
- prepared value home lookup and register-or-stack-slot kind validation,
- storage-plan value lookup and result-name validation, and
- register-vs-frame-slot storage compatibility plus frame-slot offset decoding.

The load record builder still owns load target register materialization,
stack-publication scratch selection, and result type/register-view policy.

## Suggested Next

Next coherent Step 3 packet: extract shared prepared stored-value home/storage
decoding for store values, without broadening into Step 4 stack-source
publication planning or changing immediate-store behavior.

## Watchouts

The new load-result helper intentionally keeps stack-publication scratch
register selection in `make_load_memory_instruction_record(...)` through
`make_load_result_stack_publication_scratch(...)`; do not move scratch choice,
AArch64 register view selection, opcode choice, or
`frame_slot_address(...)`/`register_indirect_address(...)` spelling into
prepared helpers. The store path has more variants than load results
(immediates, stack reloads, register storage, and frame-slot stored values), so
the next packet should preserve the existing immediate fast path and error
mapping.

## Proof

Passed. `test_after.log` contains the delegated proof output: build completed
and the focused CTest subset passed 13/13.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|dynamic_stack_fixed_slot_uses_fp_anchor|pointer_value_named_scalar_writeback_uses_computed_store_value)|cli_dump_prepared_bir_(is_prepared|exposes_contract_sections|local_arg_call_contract|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
