Status: Active
Source Idea Path: ideas/open/187_phase_e_route6_call_use_source_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 2-3
Current Step Title: Expose Or Reuse A Narrow Route 6 View / Migrate The Selected Consumer

# Current Packet

## Just Finished

Completed Steps 2 and 3 of `plan.md`: reused the selected caller's narrow
`Route6CallUseSourceIndex`, populated direct-global dependency records for the
current call arguments, and migrated
`materialize_direct_global_select_chain_call_argument(...)` to query
`bir::route6_find_call_argument_direct_global_dependency(...)` before prepared
fallback.

The prepared fallback/oracle remains
`prepare::find_prepared_call_argument_publication_source_routing(*argument_plan)`;
value homes, ABI register placement, scratch selection, and instruction
emission remain AArch64/prepared-owned.

## Suggested Next

Review whether the now-migrated direct-global call-argument consumer completes
the active Route 6 consumer-migration runbook or whether a plan-owner lifecycle
decision is needed for the next source-intent slice.

## Watchouts

The slice intentionally moved only semantic direct-global select-chain
call-argument dependency facts to Route 6. Frame-slot source-selection paths,
ABI-bound placement, helper/carrier policy, and call record spelling were not
changed.

## Proof

Passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_aarch64_call_boundary_owner)$') > test_after.log 2>&1`

Proof log: `test_after.log`.
