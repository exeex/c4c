Status: Active
Source Idea Path: ideas/open/186_phase_e_route3_memory_semantic_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Route 3 Consumer

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: selected exactly one Route 3 consumer path for
the next implementation packet.

Selected consumer path:
`src/backend/mir/aarch64/codegen/calls.cpp`
`materialize_indirect_call_callee_to_prepared_register(...)` ->
`find_prepared_indirect_callee_stored_value_source(...)`, used when an indirect
callee source comes from a same-block `LoadLocalInst` and the caller needs the
stored source value before checking direct-global select-chain dependency.

Prepared fallback/oracle:
keep
`prepare::find_prepared_same_block_load_local_stored_value_source(...)` as the
fallback and equivalence oracle. It currently returns the stored value,
store instruction index, load producer, load access, and store access used by
the caller.

Route 3 surface / adapter:
read Route 3 through `mir::find_bir_same_block_load_local_source_identity(...)`
or directly through `bir::route3_find_same_block_load_local_source(...)` with a
small adapter. The existing MIR query exposes load-local source identity but
does not expose the recovered stored value or store instruction index needed by
this caller, so Step 2 should either extend that narrow semantic identity or add
a fail-closed adapter that returns only stored-value/source relationship facts.
Do not move address formation, stack/frame layout, ABI call policy, or memory
operand records into Route 3.

## Suggested Next

Implement the selected calls-side load-local source adapter and switch only
`find_prepared_indirect_callee_stored_value_source(...)` to try Route 3 first
with prepared fallback/oracle behavior preserved.

## Watchouts

The same-block global-load value-materialization path already has Route 3 reads
before prepared fallback, so do not select it as this plan's new consumer.
Keep prepared memory/access helpers public. Reject target-address policy
movement, broad BIR rescans, name matching, expectation downgrades, or
multiple-consumer migration in one slice.

## Proof

Selection-only packet; no build or tests run and no `test_after.log` produced.
Recommended proof for the next code-changing packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_store_source_publication_plan|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call)$'`
