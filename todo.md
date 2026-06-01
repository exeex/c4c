Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Consume Prepared Addressing And Materialization Facts

# Current Packet

## Just Finished

Completed the final in-scope `plan.md` Step 2 packet in
`src/backend/mir/aarch64/codegen/memory.cpp`: published store-global stack
value publication now resolves and validates the prepared stack home in
`find_prepared_store_global_stack_publication_home(...)` before the AArch64
emitter runs. `lower_published_store_global_stack_value_publication(...)` now
consumes the prepared home and keeps only result filtering, scratch selection,
value materialization, store mnemonic/view choice, and final frame-address
emission.

## Suggested Next

Supervisor should review Step 2 for closure or handoff. After checking the
remaining store-global and local publication helpers in `memory.cpp`, I do not
see another concrete in-scope prepared addressing/materialization lookup to
migrate without broadening into value-home/storage Step 3 or stack-source Step
4 policy.

## Watchouts

`find_prepared_store_global_stack_publication_home(...)` intentionally owns
only lookup/validation of the prepared stack home. Remaining inline
`find_indexed_prepared_value_home(...)` calls in this area are tied to future
store-local coverage decisions or broader value-home/storage routing, so I left
them out of this Step 2 packet.

## Proof

Passed. Proof log: `test_after.log`.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|dynamic_stack_fixed_slot_uses_fp_anchor|pointer_value_named_scalar_writeback_uses_computed_store_value)|cli_dump_prepared_bir_(is_prepared|exposes_contract_sections|local_arg_call_contract|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
