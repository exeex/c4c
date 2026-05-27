Status: Active
Source Idea Path: ideas/open/54_aarch64_global_value_materialization_consumer_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Route `globals.cpp` Through Prepared Authority

# Current Packet

## Just Finished

Completed Step 2 implementation for `globals.cpp` GOT global-load
materialization.

Added `prepare::find_prepared_global_load_access(...)`, keyed by prepared
addressing function, block label, instruction index, and expected
`bir::LoadGlobalInst`. The query validates the prepared memory access against
the `LoadGlobal` result via `prepared_global_load_access_matches_result`,
requires a global-symbol prepared address with `symbol_name` and
`can_use_base_plus_offset`, and returns `{load_global, access}`. The existing
dispatch-shaped `prepare::find_prepared_same_block_global_load_access(...)`
now delegates to that shared query.

`make_load_global_got_materialization_instruction` now uses the prepared query
through `prepared_current_global_load_access(...)`, derives GOT eligibility
from `prepare::prepared_global_symbol_address_policy(access.address,
context.function.target_profile)`, gets the symbol with
`prepare::prepared_link_name(...)`, and uses `access.address.byte_offset` for
the final indirect load. It no longer uses `find_load_global_target`,
target-global policy, or `load_global_symbol_label` when the prepared access
can express the current `LoadGlobal`.

## Suggested Next

Delegate Step 3 for `fp_value_materialization.cpp`: route same-block FP
`LoadGlobal` materialization through prepared global-load authority without
recovering symbol identity or address policy from raw globals.

## Watchouts

The new shared query is intentionally access-validation only; callers still
choose whether they need GOT or direct lowering by consulting
`prepare::prepared_global_symbol_address_policy`. Step 3 should avoid touching
`fp_value_materialization.cpp` until delegated because it is outside this
packet.

## Proof

Ran exactly:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_cli_dump_prepared_bir_is_prepared|backend_cli_dump_prepared_bir_exposes_contract_sections)$') > test_after.log 2>&1`

Result: passed. `test_after.log` records a successful preset build and 4/4
matching tests passed.
