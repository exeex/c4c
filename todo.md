Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Value-Home, Storage, And Frame-Offset Authority Into Memory

# Current Packet

## Just Finished

Completed the pointer-value base-register resolution pass for `plan.md` Step 3
in `src/backend/mir/aarch64/codegen/memory.cpp`. Used `c4c-clang-tools` to
inspect the current helper relationships, confirmed that
`make_load_memory_instruction_record(...)` and
`make_store_memory_instruction_record(...)` still duplicated the same
pointer-value base-register resolution already wrapped by
`resolve_pointer_value_base_register(...)`, then migrated both builders onto
that shared helper.

After the migration, the named helpers have no remaining concrete in-scope
pointer-value base-register resolution duplication: load/store call the shared
resolver, the resolver owns the record mutation, and the f128 transport path
continues to call the same resolver.

## Suggested Next

Advance to `plan.md` Step 4. Reviewer report
`review/idea70_steps1_3_review.md` found the route on track and no overfit,
expectation downgrade, or target-policy leakage. No further narrow
pointer-value base-register resolution packet remains in the checked helper
set.

## Watchouts

This packet intentionally did not broaden into Step 4 stack-source publication
planning. `make_prepared_pointer_value_base_register(...)` remains the
authority for value-home/storage validation and register conversion;
`resolve_pointer_value_base_register(...)` remains the record-mutating wrapper.

## Proof

Passed. The focused proof was rerun after review and compared against the
accepted `test_before.log`: 13/13 before, 13/13 after, no new failures. The
accepted after log was rolled forward to `test_before.log`.

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|codegen_route_aarch64_(got_load_global_prepared_memory|store_global_stack_publication|local_aggregate_address_pointer_copy_publishes_frame_address|dynamic_stack_fixed_slot_uses_fp_anchor|pointer_value_named_scalar_writeback_uses_computed_store_value)|cli_dump_prepared_bir_(is_prepared|exposes_contract_sections|local_arg_call_contract|00204_stdarg_prepared_handoff_aarch64_publication))$'; } > test_after.log 2>&1
```
