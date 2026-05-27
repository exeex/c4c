# Current Packet

Status: Active
Source Idea Path: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Route select-chain/direct-global late materialization through shared authority

## Just Finished

Step 5 routed direct-global select-chain call-argument materialization through
prepared call-plan/source-value authority plus a shared direct-global
select-chain dependency query. `materialize_direct_global_select_chain_call_argument`
now requires a matching `PreparedCallArgumentPlan` source value and consumes
`find_prepared_direct_global_select_chain_dependency` before emitting late
materialization. `emit_select_chain_value_to_register` accepts that shared
dependency authority and validates the root producer/index/kind instead of
owning an AArch64-only direct-global eligibility query.

The prepared lookup helper test now covers shared direct-global select-chain
dependency facts for select roots, direct-load roots, and missing roots. The
focused AArch64 route proof covers the repaired call-argument path and nearby
selected-indirect/scalability routes.

## Suggested Next

Ask the plan owner to review whether the active runbook is complete, blocked,
or needs a follow-up Step 6 for remaining null-publication/select-chain
fallback cleanup.

## Watchouts

`find_edge_named_producer` and `unique_branch_predecessor_context` remain
defined/exported, but the repaired source-producer materialization and hazard
paths no longer call them. Load-local materialization still allows the
non-direct/null-publication prepared-memory-access route; the direct prepared
publication route now requires available source-memory authority. Block-entry
edge-copy suppression no longer drops same-register or source-memory moves
without prepared edge-publication authority. Indirect callee materialization
still has a separate direct-global select-chain check outside the call-argument
path covered by this packet.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 4/4 focused tests green. Proof log: `test_after.log`.
