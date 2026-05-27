# Current Packet

Status: Active
Source Idea Path: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Consolidate proof and regression guard expectations

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

Run Step 6 proof consolidation before closing the source idea. Confirm the
current implementation has no remaining call sites of `find_edge_named_producer`
or `unique_branch_predecessor_context`, no expectation downgrades or
unsupported-path rewrites, and no new local predecessor/name-shaped fallback
matching in the Step 2-5 commits.

Then run the supervisor-selected close-scope validation and compare canonical
`test_before.log`/`test_after.log` with `c4c-regression-guard`. The proof should
record coverage for source-producer authority, remaining null-publication
fallback behavior, load-local source-memory materialization, block-entry
move-bundle matching, and select-chain/direct-global shared authority.

If that guard passes, ask the plan owner to close/archive
`ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md`.

## Watchouts

`find_edge_named_producer` and `unique_branch_predecessor_context` remain
defined/exported, but current `src/` and `tests/` references show no call sites
outside the helper body/declarations. Treat any reintroduced caller as a close
blocker unless it is routed through a shared prepared query. Load-local
materialization still allows the non-direct/null-publication
prepared-memory-access route; the direct prepared-publication route now
requires available source-memory authority. Block-entry edge-copy suppression
no longer drops same-register or source-memory moves without prepared
edge-publication authority. Indirect callee materialization still has a
separate direct-global select-chain check outside the call-argument path
covered by Step 5, so Step 6 should confirm it is either out of this idea's
acceptance scope or already covered by the shared direct-global dependency
query.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 4/4 focused tests green. The supervisor rolled the focused
proof forward, so only `test_before.log` is currently present at this handoff.
Step 6 still needs fresh close-scope `test_after.log` and regression-guard
comparison before lifecycle closure.
