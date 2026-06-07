Status: Active
Source Idea Path: ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Expose Current-Block Join And Producer Routing Facts

# Current Packet

## Just Finished

Step 3 - Expose Current-Block Join And Producer Routing Facts completed two
compact target-neutral prepared lookup sub-slices.

- Added public
  `prepare::prepare_current_block_join_parallel_copy_instruction_routing`.
- The current-block join helper consumes the existing
  `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`, reuses prepared
  current-block join source facts, resolves instruction results through
  prepared names, regalloc, and value homes, and returns instruction-position
  routing bits for incoming-expression results and source results.
- Corrected the shared query inputs to carry `const PreparedRegallocFunction*`
  so shared routing preserves the old AArch64
  `instruction_result_prepared_value_id` fallback behavior.
- Updated AArch64 `build_current_block_join_prepared_query_routing` to consume
  the shared prepared routing bits instead of locally building value-id sets
  and membership checks in `dispatch_producers.cpp`.
- Exposed existing
  `prepare::find_prepared_direct_global_select_chain_dependency` through the
  shared prepared lookup header.
- Updated AArch64 `select_chain_contains_direct_global_load` to assemble
  prepared source-producer lookups from `BlockLoweringContext` and consume the
  shared prepared dependency fact instead of recursively walking select/cast/
  binary producers in `dispatch_producers.cpp`.
- Kept AArch64 register-read hazard policy, physical register parsing,
  lowering-context assembly, and fallback emission decisions out of prepare.
- Extended `backend_prepared_lookup_helper` coverage for current-block join
  instruction routing over incoming-expression and source result positions,
  including a result value discoverable through regalloc fallback only.
- Reused existing focused `backend_prepared_lookup_helper` coverage for the
  direct-global select-chain query, including select-root, direct-load-root,
  scalar materialization, call-argument facade, and fail-closed missing-root
  behavior.

Retained Step 2 summary:

- Added shared prepared helpers for edge-publication and parallel-copy facts:
  `prepared_edge_copy_source_facts_have_materializable_producer`,
  `prepared_edge_publication_source_home_matches_source`, and
  `prepared_edge_publication_source_memory_matches_access`.
- Updated AArch64 edge-copy lowering to consume those helpers for
  materializable source-producer checks plus source-home/source-memory
  consistency, while leaving register parsing, memory access discovery, and
  target-specific hazard policy local.
- Covered the Step 2 helpers in `backend_prepared_lookup_helper`, including
  available/fail-closed materializable producer facts, matching and mismatched
  source homes, matching source memory, mismatched memory offsets, and unnamed
  prepared memory access results; AArch64 dispatch coverage was extended for
  fail-closed direct load edge publication memory mismatch.

## Suggested Next

Delegate Step 4 boundary audit for the remaining producer logic in
`dispatch_producers.cpp`, especially `value_publication_may_read_register_index`,
because the remaining same-block producer recursion is tied to AArch64
register hazard policy and physical register read checks.

## Watchouts

- No predecessor rescans, BIR-name matching, same-block named-case shortcuts,
  expectation downgrades, or AArch64 register-policy moves were added.
- AArch64 still assembles prepared lookup inputs from `BlockLoweringContext`;
  the moved Step 3 facts are target-neutral current-block join instruction
  routing and direct-global select-chain dependency.
- Query callers that need AArch64-equivalent instruction-result resolution
  should pass the prepared regalloc function through the shared query inputs;
  passing indexed value-home lookups alone intentionally keeps the indexed
  lookup fail-closed behavior.
- The remaining `value_publication_may_read_register_index` recursion should
  not move to prepare without a separate design because it depends on current
  block-entry publication semantics and AArch64 register alias/read policy.

## Proof

Ran exactly:

`(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure) > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and both targeted
tests passing:

- `backend_aarch64_instruction_dispatch`
- `backend_prepared_lookup_helper`
