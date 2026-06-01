Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Memory Authority Duplication

# Current Packet

## Just Finished

Completed `plan.md` Step 1 mapping for
`src/backend/mir/aarch64/codegen/memory.cpp`.

Authority classification from the clang-tool symbol inventory:

- prepared-addressing:
  `make_memory_record_from_prepared_access(...)`,
  `make_prepared_memory_operand_record(...)` for load/store local/global,
  `prepared_store_global_addressing(...)`,
  `prepared_global_symbol_from_link_name(...)`, and
  `emit_pointer_base_plus_offset_to_register(...)` are the address-authority
  boundary. Existing code already reads `PreparedMemoryAccess`,
  `PreparedAddressingFunction`, and pointer/global materialization facts, but
  still has scattered local validation and address-materialization decisions
  around pointer-base-plus-offset and local address stores.
- value-home/storage:
  `indexed_value_home_id(...)`, `require_indexed_value_home_id(...)`,
  `make_load_memory_instruction_record(...)`,
  `make_store_memory_instruction_record(...)`,
  `find_memory_return_abi_register(...)`,
  `prepared_or_emitted_store_value_register(...)`, and
  `lower_*_value_publication*` decide whether prepared homes and storage plans
  are register, stack-slot, immediate, or pointer-base-plus-offset backed.
  These decisions should remain fact lookups; AArch64 should only turn the
  selected home/storage into registers, scratch copies, or stores.
- frame-offset:
  `find_frame_slot_by_slot_id(...)`,
  `make_frame_slot_operand_from_stack_slot(...)`,
  `resolve_frame_slot_memory_offset(...)`,
  `rewrite_local_address_store_value(...)`, direct `frame_slot_address(...)`
  calls using prepared `offset_bytes`, and stack-object lookup helpers own the
  duplicated offset boundary. The authoritative offset is prepared
  `PreparedFrameSlot`/`PreparedValueHome::offset_bytes`; AArch64-local work is
  base-register choice and final address spelling.
- aggregate stack-source:
  `plan_store_local_source_publication(...)`,
  `lower_store_local_value_publication(...)`,
  `lower_pending_store_global_stack_value_publications(...)`, and
  `lower_store_global_value_publication_from_plan(...)` route store-source
  publications through `PreparedStoreSourcePublicationPlan`, but memory.cpp
  does not yet consume `PreparedAggregateStackSourceAuthority` directly.
- typed stack-source:
  stack-homed scalar publication paths in
  `lower_store_local_value_publication(...)`,
  `lower_published_store_global_stack_value_publication(...)`, and
  `future_store_local_stack_value_publication_covers_instruction(...)` use
  prepared value homes and store-source publication plans, but memory.cpp does
  not yet consume `PreparedTypedStackSourcePublication` directly.
- target-local instruction policy:
  `scalar_load_mnemonic(...)`, `scalar_store_mnemonic(...)`,
  `scalar_*_register_view(...)`, `make_prepared_register_operand(...)`,
  `machine_opcode_from_memory_instruction(...)`, `memory_address(...)`,
  scratch-register selection, direct/scratch base-plus-offset lowering, and
  va_list field addressing remain AArch64 policy rather than shared prepared
  authority.

## Suggested Next

First executable Step 2 packet: in `memory.cpp`, centralize the prepared
addressing/materialization boundary for store-local address publication by
making `rewrite_local_address_store_value(...)` and
`apply_stack_layout_to_memory_record(...)` consume a single prepared
frame-address/materialization lookup result before any AArch64 frame-slot
operand rewrite. Keep `resolve_frame_slot_memory_offset(...)`,
base-register policy, scratch-register choice, load/store mnemonic selection,
and final address spelling local. Suggested proof for that code packet:
fresh build plus the focused AArch64 memory-lowering subset selected by the
supervisor.

## Watchouts

Do not treat `PreparedStoreSourcePublicationPlan` as a replacement for the
aggregate or typed stack-source facts in Step 4; those are separate prepared
authorities. Also keep `memory_address(...)`, `frame_slot_address(...)`,
relocation spelling, mnemonic choice, register view conversion, immediate
legality, and va_list field cursor updates target-local.

## Proof

`git diff --check -- todo.md` passed for this mapping-only packet. The
delegated proof is logless and does not produce `test_after.log`.
