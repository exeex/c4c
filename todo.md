Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared Addressing And Materialization Facts

# Current Packet

## Just Finished

Completed the first executable `plan.md` Step 2 packet in
`src/backend/mir/aarch64/codegen/memory.cpp`: local address store publication
now consumes the prepared frame-address/materialization lookup once in
`find_prepared_local_address_store_value(...)` before
`apply_stack_layout_to_memory_record(...)` resolves frame-slot offsets or
rewrites the store value operand. `rewrite_local_address_store_value(...)` now
only applies the already-selected prepared operand.

## Suggested Next

Continue Step 2 by looking for the next prepared addressing/materialization
fact that is still consumed inside an AArch64 rewrite or validation helper, and
move that fact lookup to the same pre-rewrite boundary without changing
target-local address spelling, scratch selection, or mnemonic policy.

## Watchouts

Keep `resolve_frame_slot_memory_offset(...)` as target-local frame-slot offset
application, and keep base-register policy, scratch-register selection,
mnemonic selection, and final memory-address spelling in AArch64 codegen. This
packet intentionally did not broaden into Step 3/4 aggregate or typed
stack-source publication authority.

## Proof

Passed. Proof log: `test_after.log`.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|dynamic_stack_fixed_slot_uses_fp_anchor|pointer_value_named_scalar_writeback_uses_computed_store_value)|cli_dump_prepared_bir_(is_prepared|exposes_contract_sections|local_arg_call_contract|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
