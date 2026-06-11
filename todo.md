Status: Active
Source Idea Path: ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build the Evidence Inventory

# Current Packet

## Just Finished

Step 1: Build the Evidence Inventory completed for Phase C2.

Required artifacts read:

- `ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md`
- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `ideas/closed/201_phase_b2_selected_route_extension_schema_readiness_audit.md`
- `ideas/closed/202_route3_memory_source_identity_adapter.md`
- `ideas/closed/203_route4_publication_identity_adapter.md`
- `ideas/closed/204_route5_edge_join_source_adapter.md`
- `ideas/closed/205_route6_call_use_source_adapter.md`
- `ideas/closed/206_route7_comparison_provenance_diagnostic_oracle.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `test_baseline.log`

Missing required artifacts: none.

Ambiguous inputs: none blocking Step 1. The main C2 caution is evidentiary:
the Route 3 through Route 7 closures prove bounded adapter correctness or
diagnostic/oracle behavior, but they do not by themselves prove public prepared
API contraction or aggregate `PreparedFunctionLookups` retirement.

Inventory rows:

| Route | Selected reader or diagnostic row | Prepared surfaces touched | Proof scope captured | Evidence categories | Inventory status |
| --- | --- | --- | --- | --- | --- |
| Route 3 memory/source | Selected Route 3 load-local adapter; earlier map also cites the AArch64 indirect-callee stored-value source consumer as prior selected-reader evidence. | `memory_accesses`, `PreparedMemoryAccessLookups`, prepared load-local source-home helpers, target-addressing fallback, memory/frame/stack oracles. | Backend before/after close proof with 180 passed and no new failures; closure says success, absence, mismatch, non-memory, ambiguity, fallback, and non-regressive baseline behavior were required. | Adapter correctness; oracle equivalence for fallback behavior. Not consumer exhaustion or contraction evidence. | Prepared memory/source helpers remain public fallback/oracle and target-policy surfaces until every production, wrapper, and oracle consumer is accounted for. |
| Route 4 publication | Selected AArch64 call-boundary current-block publication source reader. | Current/block-entry publication helpers, `edge_publications`, `PreparedEdgePublicationLookups`, prepared publication mechanics, wrapper/debug output surfaces. | Closure records migration of semantic publication identity only; proof expectations include absent, mismatch, duplicate/ambiguous, wrong-reference, and unchanged output where affected. | Adapter correctness; oracle equivalence for output/fallback. Not broad consumer migration or contraction evidence. | Publication mechanics, move/home/storage policy, stack-source extension, block-order emission, wrappers, and emitted strings remain retained prepared/target policy or public oracle surfaces. |
| Route 5 edge/join-source | `build_current_block_join_prepared_query_routing` selected as the bounded reader boundary, using indexed Route 5 current-block join-source validation through `mir::find_bir_current_block_join_source_identity`. | `PreparedEdgePublicationLookups`, `PreparedEdgePublicationSourceProducerLookups`, indexed edge-publication/source-producer helpers, current-block join-source helpers, move-bundle lookup surfaces. | Backend before/after close proof with 180 passed and no new failures; closure names fail-closed Route 5 validation and retained prepared scheduling/storage/wrapper/branch/final-record authority. | Adapter correctness; oracle equivalence for fail-closed/fallback behavior. Not move/edge publication contraction evidence. | Edge/join source identity moved only for one semantic boundary; move scheduling, homes/storage, branch policy, wrappers, final edge-copy records, and emitted output remain retained target/prepared policy or oracle surfaces. |
| Route 6 call-use source | Selected AArch64 scalar call argument source-producer Route 6 adapter. | `call_plans`, `PreparedCallPlanLookups`, call argument source-producer materialization helpers, prepared call-plan fallback, x86 `ConsumedPlans` compatibility context. | Backend close proof with 180 passed before/after; closure says adapter accepts Route 6 only when source id/name/value, unique key, current-block producer pointer validity, and scalar materialization availability agree, otherwise falls back to prepared call-plan lookup. | Adapter correctness; oracle equivalence for route/prepared agreement and fallback. Partial consumer migration for one argument-source read, but not contraction evidence. | Prepared call plans remain authoritative for ABI, layout, helper protocols, policy, storage, movement, call records, wrappers, and emitted output. |
| Route 7 comparison/provenance | Materialized-condition provenance reader selected as the Route 7 diagnostic/oracle surface. | Route 7 materialized-condition/fused-compare comparison helper surfaces, prepared fused-compare and materialized-condition fallbacks, branch-policy oracles, route-debug/printer/debug/string-authority surfaces. | Backend proof with 180 passed before/after; closure records absent, invalid, duplicate, mismatched fail-closed validation and preserved AArch64/prepared branch authority. | Adapter correctness; diagnostic/oracle equivalence. Not consumer exhaustion or diagnostic contraction evidence. | Prepared/AArch64 authority remains for branch targets, final branch spelling, condition suffix mapping, fused legality, hazards, emitted-register state, final instruction order, final assembler rows, and expected strings. |

Baseline evidence read:

- `test_baseline.log` records baseline commit
  `ec54d476ac899c9c8600d7451fde8227fa5b3ee4`, baseline regex
  `<full-suite>`, and 3428/3428 tests passed.
- This baseline is proof-scope context only. It is not standalone contraction
  evidence and must not be used to claim cache/API readiness.

## Suggested Next

Execute Step 2 from `plan.md`: classify Route 3 through Route 7 touched
surfaces in
`docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`
using the working-model classifications, with one row per selected surface and
no contraction claim based only on adapter greenness.

## Watchouts

- This is analysis-only unless a later packet is explicitly delegated to create
  bounded follow-up ideas.
- Do not claim cache/API contraction from adapter greenness alone.
- Step 1 found no actual contraction evidence yet; the evidence is adapter
  correctness, diagnostic/oracle equivalence, or one-reader consumer migration
  evidence with retained prepared fallback.
- Do not weaken test contracts, downgrade supported paths, or treat full-suite
  greenness as contraction readiness.
- Keep `ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md`
  unchanged unless durable source intent genuinely changes.

## Proof

Docs-only analysis packet; no build or CTest command was delegated or run.
Verification performed by checking every required Read First artifact exists,
reading the evidence inventory inputs, and recording missing/ambiguous input
status above. No `test_after.log` was produced for this docs-only packet.
