Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Value-Home, Storage, And Frame-Offset Authority Into Memory

# Current Packet

## Just Finished

Completed the initial mapping packet for `plan.md` Step 3 in
`src/backend/mir/aarch64/codegen/memory.cpp` without touching implementation
files. AST-backed inventory shows the remaining value-home/storage/frame-offset
authority clusters are:

- Prepared home lookup authority: inline
  `prepare::find_indexed_prepared_value_home(...)` consumers remain in pointer
  value load publication, stack-homed pointer load/store writeback,
  pointer-base-plus-offset store-local publication, pointer materialization,
  store-global stack publication, and future store-local coverage checks.
- Storage-plan authority: `find_storage_plan_value(...)` is consumed by
  `resolve_pointer_value_base_register(...)`,
  `make_load_memory_instruction_record(...)`,
  `make_store_memory_instruction_record(...)`, and
  `find_va_list_field_address(...)`; the load/store record builders still
  directly pair prepared homes with storage encodings for pointer bases,
  load results, and stored values.
- Decoded home-storage authority: load/store record creation currently decides
  register vs stack vs immediate storage combinations inline, including stack
  result publication scratch selection and store-from-stack memory operands.
- Frame-offset authority: `resolve_frame_slot_memory_offset(...)` applies
  prepared stack-layout offsets inside `apply_stack_layout_to_memory_record(...)`,
  while `find_prepared_local_address_store_value(...)` uses
  `find_indexed_prepared_frame_address_offset_for_value_id(...)` to rewrite
  local address stores to prepared frame-slot operands.

Prepared facts should continue to decide the authoritative home, storage
encoding, stack slot, and frame offset. AArch64-local policy should stay limited
to register view/class conversion, scratch register selection, load/store
mnemonics, immediate legality, and final address spelling.

## Suggested Next

First executable Step 3 packet: extract a small prepared storage/home decoding
helper for pointer-value base registers and use it from both
`resolve_pointer_value_base_register(...)` and the duplicated pointer-base
branches in `make_load_memory_instruction_record(...)` and
`make_store_memory_instruction_record(...)`. The helper should validate the
prepared value id/name, require a prepared register home plus register storage
encoding for pointer bases, and return the prepared base register or the
existing `PreparedMemoryOperandRecordError`.

## Watchouts

Keep the first Step 3 implementation packet away from Step 4 stack-source
publication planning: `plan_stack_homed_pointer_store_writeback(...)` and
`plan_pointer_base_plus_offset_store_local_publication(...)` consume publication
plans and should not be broadened while folding pointer-base home/storage
decoding. Do not move AArch64 register view selection, scratch choice, opcode
choice, or `frame_slot_address(...)`/`register_indirect_address(...)` spelling
into prepared helpers.

## Proof

Passed. Logless documentation check; no `test_after.log` was produced.

Command:

```sh
git diff --check -- todo.md
```
