Status: Active
Source Idea Path: ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Characterize Dispatch Producer Contract Residue

# Current Packet

## Just Finished

Step 1 - Characterize Dispatch Producer Contract Residue complete.

Helper-to-contract map from AST queries plus narrow reads:

- `dispatch_edge_copies.cpp::prepared_edge_publication_producer_block_context`:
  local AArch64 BIR block/context reconstruction from a prepared block label.
  Shared contract already exists for prepared labels/names through
  `prepare::prepared_block_label`; missing shared fact is a reusable
  prepared-block-to-BIR-block/context resolver so dispatch files do not keep
  spelling-based block lookup authority.
- `dispatch_edge_copies.cpp::prepared_edge_publication_producer_context` and
  `prepared_edge_source_producer_context`: local validation that prepared
  producer kind, block label, instruction index, and typed producer pointer
  agree with the BIR instruction. Shared producer facts already exist in
  `prepare::PreparedEdgePublication`, `prepare::PreparedEdgeCopySourceFacts`,
  and `prepare::PreparedEdgePublicationSourceProducer`; missing shared fact is
  an exported producer-instruction validation/query returning the producer
  instruction plus block label/index, independent of AArch64 emission.
- `dispatch_edge_copies.cpp::prepared_edge_named_source_producer_context`:
  local wrapper over
  `prepare::find_indexed_prepared_edge_publication_source_producer` plus local
  context validation. Shared lookup exists; missing shared helper is the
  resolved producer-context form that avoids each backend revalidating pointer
  identity.
- `dispatch_edge_copies.cpp::prepared_edge_copy_source_facts_have_materializable_producer`:
  local boolean over `PreparedEdgeCopySourceFactsStatus::Available` and
  non-`Unknown`/non-`Immediate` producer kind. Candidate Step 2 shared helper:
  `prepare::prepared_edge_copy_source_has_materializable_producer(facts)`.
- `dispatch_edge_copies.cpp::prepared_publication_source_home_matches_source`,
  `prepared_publication_source_memory_matches_access`,
  `prepared_publication_source_memory_access`, and
  `prepared_publication_source_register`: local validation of publication
  source home/memory/register consistency before AArch64 load/register
  materialization. Shared publication fields already contain the contract
  (`source_home`, `source_memory_access`, copied source-memory fields);
  candidate shared facts are source-home-match and source-memory-access-match
  predicates, while AArch64 register parsing must stay local.
- `dispatch_edge_copies.cpp::edge_value_publication_may_read_register_index`:
  AArch64-specific hazard check, but its producer traversal still depends on
  local prepared producer-context validation. Sharedable part is "which
  prepared source producers/dependencies may be read"; AArch64 register
  alias/index policy stays local.
- `dispatch_edge_copies.cpp::should_emit_block_entry_edge_copy_move` and
  `lower_predecessor_select_parallel_copy_sources`: already consume
  `prepare::prepare_edge_copy_source_facts`,
  `prepare::prepare_block_entry_parallel_copy_edge_source_facts`,
  `prepare::prepared_edge_publication_matches_parallel_copy_move_source`, and
  `prepare::prepared_edge_publication_redundant_block_entry_parallel_copy_move`.
  Remaining residue is the local materializable-producer predicate and local
  producer-context validation before edge-publication emission.
- `dispatch_producers.cpp::prepared_publication_source_producer_for_value`:
  AArch64 wrapper around
  `prepare::find_indexed_prepared_edge_publication_source_producer`, including
  fallback construction with
  `prepare::make_prepared_edge_publication_source_producer_lookups`. Contract
  is mostly shared; candidate improvement is a small context-level helper so
  `prepared_lookups` fallback construction is not duplicated in dispatch.
- `dispatch_producers.cpp::prepared_same_block_publication_source_producer`
  and `prepared_same_block_select_producer`: wrappers over
  `prepare::find_prepared_same_block_scalar_producer` plus local select-only
  narrowing. Shared same-block producer query exists; select-specific contract
  could be represented as a shared `PreparedSameBlockSelectProducer`/kind
  predicate if Step 3 wants to remove local select pointer checks.
- `dispatch_producers.cpp::prepare_current_block_join_parallel_copy_source_facts`,
  `prepared_query_current_block_join_parallel_copy_source`,
  `build_current_block_join_prepared_query_routing`,
  `current_block_join_prepared_query_incoming_expression`, and
  `current_block_join_prepared_query_source`: shared query already exists as
  `prepare::prepare_current_block_join_parallel_copy_source_facts` with
  `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`; local residue is
  context/input assembly and routing arrays keyed by instruction result value
  IDs.
- `dispatch_producers.cpp::block_entry_move_clobbers_current_join_publication`:
  local AArch64 register-alias consumer of
  `prepare::collect_prepared_block_entry_publications` and
  `prepare::prepared_block_entry_publication_available`. Shared publication
  query exists; register parsing and alias checks should remain AArch64-local.
- `dispatch_producers.cpp::value_publication_may_read_register_index`:
  current-block analogue of the edge hazard traversal. It consumes
  `prepared_same_block_publication_source_producer`; sharedable part is
  producer/dependency graph classification, not AArch64 register-index reads.

Named existing shared prepared queries relevant to Step 2/3:

- `prepare::make_prepared_edge_publication_source_producer_lookups`
- `prepare::find_indexed_prepared_edge_publication_source_producer`
- `prepare::find_prepared_same_block_scalar_producer`
- `prepare::find_prepared_fused_compare_operand_producer`
- `prepare::find_prepared_materialized_condition_producer`
- `prepare::find_prepared_global_load_access`
- `prepare::find_prepared_same_block_global_load_access`
- `prepare::find_prepared_same_block_load_local_stored_value_source`
- `prepare::prepare_edge_copy_source_facts`
- `prepare::prepare_block_entry_parallel_copy_edge_source_facts`
- `prepare::prepared_edge_publication_matches_parallel_copy_move_source`
- `prepare::prepared_edge_publication_redundant_block_entry_parallel_copy_move`
- `prepare::prepare_current_block_join_parallel_copy_source_facts`
- `prepare::collect_prepared_block_entry_publications`
- `prepare::prepared_block_entry_publication_available`

Candidate missing or too-private shared facts:

- Public materializable-producer predicate for `PreparedEdgeCopySourceFacts`.
- Public prepared producer-instruction/context validation for
  `PreparedEdgePublication` and `PreparedEdgePublicationSourceProducer`.
- Public source-home/source-memory consistency predicates for edge
  publications.
- Public context/input builder or query wrapper for current-block join
  parallel-copy source facts, so dispatch code does not rebuild lookups
  ad hoc.
- Optional shared select-specific same-block producer result if Step 3 removes
  local `SameBlockSelectProducer` pointer checks.

## Suggested Next

Delegate Step 2 as a narrow implementation packet: move the lowest-risk
edge-publication/parallel-copy residue into shared prepared helpers, starting
with a public materializable-producer predicate for
`PreparedEdgeCopySourceFacts` and, if still compact, a public
publication/source-producer validation helper consumed by
`dispatch_edge_copies.cpp`.

Recommended owned files for the first implementation packet:
`src/backend/prealloc/prepared_lookups.hpp`,
`src/backend/prealloc/prepared_lookups.cpp`,
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`,
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`,
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, and
`todo.md`.

## Watchouts

- Do not edit implementation files during Step 1 unless the delegated packet
  explicitly authorizes it.
- Do not reopen idea 47 or idea 64 repairs without current-code evidence of a
  new uncovered gap.
- Reject predecessor rescans, BIR-name matching, same-block named-case
  matching, and select-chain special cases as permanent authority.
- Keep AArch64 instruction spelling, target register policy, memory operand
  emission, branch emission, and true machine-register hazard checks local.
- Any progress claim must cover both the edge-publication/parallel-copy family
  and the current-block producer or join-routing family before closure.
- Do not move AArch64 register parsing/aliasing into `prepare`; only move
  architecture-neutral producer/source-fact validation.
- `find_bir_block` currently lives in `dispatch_producers.cpp` and is declared
  for `dispatch_edge_copies.cpp`; replacing it with shared authority needs a
  careful ownership boundary because `prepare` should not own AArch64 lowering
  contexts.
- Existing `backend_aarch64_instruction_dispatch` cases already cover prepared
  root emission, dependency hazards, select roots, block-entry redundancy, and
  indexed block-entry publication. Add assertions only where the helper move
  weakens direct coverage.

## Proof

No build/test proof required for this characterization-only packet; no
`test_after.log` was produced.

Supervisor-ready proof recommendations for Step 2/3:

- Shared edge-publication/parallel-copy helper proof:
  `ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure`
  with focused coverage in
  `verify_edge_publication_shared_source_and_parallel_copy_facts` and
  `verify_current_block_join_parallel_copy_source_query`.
- AArch64 edge-publication behavior proof:
  `ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure`
  with attention to `edge_publication_dependency_uses_prepared_root_producer`,
  `prepared_root_emission_uses_producer_context_for_operands`,
  `prepared_select_root_emission_uses_prepared_producer_boundary`,
  `prepared_select_root_nested_select_uses_chain_labels`,
  `block_entry_edge_copy_redundancy_uses_prepared_publication_authority`, and
  `block_entry_publication_register_uses_indexed_value_identity`.
- Current-block/join-routing proof:
  keep `backend_prepared_lookup_helper` coverage for
  `prepare_current_block_join_parallel_copy_source_facts` and pair it with
  `backend_aarch64_instruction_dispatch` if `dispatch_producers.cpp` routing
  changes.
- If Step 2 touches headers or shared prepared lookup implementation, run the
  same command set after a fresh build or compile proof selected by the
  supervisor.
