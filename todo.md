Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Value-Home, Storage, And Frame-Offset Authority Into Memory

# Current Packet

## Just Finished

Completed the next executable packet for `plan.md` Step 3 in
`src/backend/mir/aarch64/codegen/memory.cpp`. Extracted
`decode_prepared_stored_value_storage(...)` as the shared prepared store-value
home/storage decoder used by `make_store_memory_instruction_record(...)`,
preserving the existing
`PreparedMemoryOperandRecordError` mapping while centralizing:

- required stored value id/name checks,
- prepared value home lookup and register/stack-slot/rematerializable-immediate
  kind validation,
- storage-plan value lookup and stored-name validation, and
- register, rematerializable-immediate, frame-slot, and register-home/frame-slot
  storage classification plus prepared stack offset decoding.

The store record builder still owns immediate operand construction, stack-slot
memory operand construction, destination-base policy for register-home
frame-slot storage, and AArch64 register materialization.

## Suggested Next

Next coherent Step 3 packet: collapse the duplicated prepared pointer-value
base-register resolution in the load/store memory instruction builders onto the
existing shared helper, while keeping opcode/address spelling and stack-source
publication planning out of scope.

## Watchouts

The store path intentionally keeps the unprepared immediate fast path before the
prepared stored-value decoder. `PreparedStoredValueStorageKind` classifies
register-home/frame-slot storage, but `make_store_memory_instruction_record(...)`
still decides whether the destination base permits the value-home register
reload path. Preserve the current `UnsupportedStoredStorage` versus
`RegisterConversionFailed` split for unusual home/storage combinations.

## Proof

Passed. `test_after.log` contains the delegated proof output: build completed
and the focused CTest subset passed 13/13.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|dynamic_stack_fixed_slot_uses_fp_anchor|pointer_value_named_scalar_writeback_uses_computed_store_value)|cli_dump_prepared_bir_(is_prepared|exposes_contract_sections|local_arg_call_contract|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
