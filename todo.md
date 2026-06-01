Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared Addressing And Materialization Facts

# Current Packet

## Just Finished

Completed another `plan.md` Step 2 packet in
`src/backend/mir/aarch64/codegen/memory.cpp`: pointer-base-plus-offset store
local publication now resolves the prepared base symbol/base-home
materialization in
`find_prepared_pointer_base_plus_offset_materialization(...)` before the
AArch64 emitter runs. `emit_pointer_base_plus_offset_to_register(...)` now
consumes the prepared materialization and keeps only target-local register
spelling, offset arithmetic, and frame-address emission.

## Suggested Next

Continue Step 2 by checking the remaining store-global and local publication
helpers for one more prepared addressing/materialization lookup that can move
to a pre-rewrite prepared-boundary helper without changing mnemonic, scratch,
base-register, or address spelling policy.

## Watchouts

`find_prepared_pointer_base_plus_offset_materialization(...)` intentionally
owns only lookup/validation of the prepared pointer materialization facts.
Keep `emit_pointer_base_plus_offset_to_register(...)` as the AArch64 emission
boundary for register names, add/sub choice, stack loads, and final
frame-address spelling. The existing direct global-symbol path in
`lower_pointer_base_plus_offset_store_local_publication(...)` was left in
place.

## Proof

Passed. Proof log: `test_after.log`.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|dynamic_stack_fixed_slot_uses_fp_anchor|pointer_value_named_scalar_writeback_uses_computed_store_value)|cli_dump_prepared_bir_(is_prepared|exposes_contract_sections|local_arg_call_contract|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
