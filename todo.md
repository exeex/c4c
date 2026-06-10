Status: Active
Source Idea Path: ideas/open/151_phase_a_bir_normalization_candidate_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Initialize Phase Artifact And Evidence Inventory

# Current Packet

## Just Finished

Step 1 of `plan.md` initialized
`docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` with the required
Phase A section skeleton and initial evidence inventory.

Inspected source surfaces:

- `docs/bir_prealloc_fusion/README.md`
- `ideas/open/151_phase_a_bir_normalization_candidate_audit.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/comparison.hpp`
- `src/backend/prealloc/formal_publications.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/*.cpp`
- `src/backend/bir/lir_to_bir/memory/*.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`
- `src/backend/mir/aarch64/codegen/atomics.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`

Inspected closure notes:

- `ideas/closed/130_aarch64_dispatch_family_post_contract_layout_audit.md`
- `ideas/closed/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md`
- `ideas/closed/132_aarch64_dispatch_lookup_thin_helper_surface_trim.md`
- `ideas/closed/133_shared_prepared_fact_query_surface_extraction.md`
- `ideas/closed/134_shared_select_chain_same_block_dependency_queries.md`
- `ideas/closed/135_shared_current_block_entry_publication_query.md`
- `ideas/closed/136_bir_prealloc_prepared_query_surface_simplification_audit.md`
- `ideas/closed/137_select_chain_public_owner_cleanup.md`
- `ideas/closed/138_call_plan_lookup_ownership_cleanup.md`
- `ideas/closed/139_addressing_lookup_ownership_cleanup.md`
- `ideas/closed/140_edge_copy_facade_split.md`
- `ideas/closed/141_prepared_lookups_residual_owner_audit.md`
- `ideas/closed/142_value_home_move_bundle_lookup_ownership.md`
- `ideas/closed/143_stack_layout_id_lookup_helpers_owner.md`
- `ideas/closed/144_source_producer_same_block_materialization_owner.md`
- `ideas/closed/145_current_block_join_fact_routing_split.md`
- `ideas/closed/146_call_argument_materialization_call_owner.md`
- `ideas/closed/147_comparison_prealloc_fact_owner.md`
- `ideas/closed/148_same_block_load_local_stored_value_owner.md`
- `ideas/closed/149_residual_prepared_lookup_include_cleanup.md`
- `ideas/closed/150_edge_publication_lookup_declaration_owner.md`

## Suggested Next

Execute Step 2: classify BIR-normalization candidates in the artifact,
starting with same-block producer/materialization, select-chain dependency,
current-block publication consumption, edge-publication source identity,
call-boundary source/dependency identity, memory/access source identity, and
comparison/materialized-condition producer identity.

## Watchouts

- This plan is analysis-only; do not edit implementation files or test
  expectations.
- Treat target-specific placement, register spelling, scratch policy, final
  emission, and target addressing facts as reject candidates unless a separate
  target-neutral semantic relationship is proven.
- Step 1 found mixed fact groups in publication, call, and memory/addressing
  surfaces; classify source/value/access identity separately from homes,
  offsets, ABI/register placement, scratch, and target addressing choices.

## Proof

Analysis-only/docs packet; no build required. Verified by inspecting the new
artifact skeleton/evidence inventory and by checking the final diff touches
only `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` and
`todo.md`.
