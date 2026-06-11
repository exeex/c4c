Status: Active
Source Idea Path: ideas/open/187_phase_e_route6_call_use_source_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Route 6 Call-Use Consumer

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: selected the AArch64 direct-global select-chain
call-argument materializer as the one Route 6 call-use consumer path.

Selected path: `lower_scalar_call_argument_producers(...)` in
`src/backend/mir/aarch64/codegen/calls.cpp` calls
`materialize_direct_global_select_chain_call_argument(...)` in
`src/backend/mir/aarch64/codegen/select_materialization.cpp` for one call
argument source class.

Prepared fallback/oracle: keep
`prepare::find_prepared_call_argument_publication_source_routing(*argument_plan)`
and its `direct_global_select_chain_dependency` as the fallback/oracle. The
current raw BIR helper
`bir_call_argument_direct_global_select_chain_dependency(...)` is the semantic
read to replace or wrap.

Route 6 query surface: use
`bir::route6_find_call_argument_direct_global_dependency(...)`, keyed by current
block, call instruction index, callee, and argument index. A narrow adapter is
needed in the selected path to provide a `Route6CallUseSourceIndex` or validated
direct-global dependency view to `select_materialization.cpp` without moving
value homes, register choice, scratch registers, or machine emission policy into
Route 6.

## Suggested Next

Expose/reuse the narrow Route 6 direct-global call-argument dependency adapter
for `materialize_direct_global_select_chain_call_argument(...)`, then migrate
that one semantic dependency read to Route 6-first with prepared fallback.

## Watchouts

Keep the migration limited to direct-global select-chain call-argument source
facts. Do not include frame-slot source-selection paths such as
`materialize_missing_frame_slot_call_arguments(...)`; those depend on
ABI/layout-bound source selections that Route 6 correctly reports as
`AbiBoundExcluded`. Keep value-home checks, ABI register placement, scratch
selection, and final machine instruction construction AArch64/prepared-owned.

## Proof

Selection-only packet; no build/tests run and no `test_after.log` produced.
Recommended next implementation proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_aarch64_call_boundary_owner)$'`.
