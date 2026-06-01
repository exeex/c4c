Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared Stack-Source Authorities

# Current Packet

## Just Finished

Completed the first executable `plan.md` Step 4 typed stack-source packet in
`src/backend/mir/aarch64/codegen/memory.cpp`. The load-result stack-publication
path now looks up a unique prepared edge publication for local/global load
results from `context.function.prepared_lookups->edge_publications`, calls
`prepare::prepare_same_width_i32_stack_source_publication(...)`, and lets an
available `PreparedTypedStackSourcePublication` supply the destination register
placement before falling back to reserved AArch64 scratch selection.

## Suggested Next

Next Step 4 packet: inspect whether aggregate stack-source publication should
consume `prepare::PreparedAggregateStackSourceAuthority` through an existing
prepared edge-publication boundary in `memory.cpp`, keeping aggregate copy
transport separate from the same-width i32 typed path.

## Watchouts

The typed packet uses only a unique prepared publication whose predecessor is
the current block and whose source producer pointer matches the load being
lowered; ambiguous or absent publications intentionally fall back to the prior
scratch-register path. Public `make_prepared_frame_slot_load_memory_instruction_record(...)`
callers still have no edge-publication parameter and therefore retain the old
fallback behavior.

## Proof

Passed. Log: `test_after.log`.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|pointer_select_aggregate_byte_copy|variadic_aggregate_overflow_byte_copy|dynamic_stack_fixed_slot_uses_fp_anchor)|prepared_lookup_helper|riscv_prepared_edge_publication|cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$'; } > test_after.log 2>&1
```
