Status: Active
Source Idea Path: ideas/open/186_phase_e_route3_memory_semantic_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The Selected Consumer

# Current Packet

## Just Finished

Repaired the Steps 2/3 Route 3 stored-value source coverage after the broader
four-test run exposed a `backend_prepared_lookup_helper` failure. The Route 3
helper and selected calls consumer behavior were preserved: the selected
consumer still tries the narrow BIR stored-value view first and falls back to
`prepare::find_prepared_same_block_load_local_stored_value_source(...)` when
Route 3 lacks range authority.

The failing test was comparing Route 3's stable load-memory identity against
`PreparedSameBlockLoadLocalStoredValueSource::load_producer`, which is not a
stable returned pointer from the prepared helper. The owned BIR and MIR tests now
compare the Route 3 load-memory instruction index against the stable prepared
`load_access->inst_index` fact while keeping the exact same-slot stored-value
and store-index agreement checks.

## Suggested Next

Supervisor should review and commit the completed Steps 2/3 slice, including
the stored-value source helper, selected consumer migration, focused coverage,
and this repair.

## Watchouts

Route 3 deliberately fails closed for missing/ambiguous local range authority;
the prepared fallback remains necessary for selected calls-side recovery when
prepared addressing/stack-layout facts can still prove the source. The selected
consumer currently uses only `stored_value` and `store_instruction_index` from
the returned prepared-shaped record. Tests should not rely on the prepared
helper result's `load_producer` pointer; use stable prepared access facts when a
load instruction index is needed.

## Proof

Passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_store_source_publication_plan|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call)$') > test_after.log 2>&1`

Proof log: `test_after.log`.
