# Phase C2 Selected Adapter Cache Contraction Readiness

Status: Step 2 route-specific surface classification drafted.

Source idea:
`ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md`

## Scope

This document records whether the Phase B2 selected Route 3 through Route 7
adapter and diagnostic closures make any prepared lookup, cache, diagnostic, or
API surface ready for contraction. Step 2 is limited to route-specific touched
surfaces. Aggregate `PreparedFunctionLookups`, `PreparedBirModule`, diagnostic
authority, final follow-up decisions, and D2 guidance are classified in later
steps.

The classification rule is conservative: adapter greenness, backend CTest
greenness, and existing full-suite baseline success are evidence for bounded
adapter correctness only. They are not contraction evidence unless the row also
shows that production, printer/debug, target-wrapper, oracle, policy-sensitive,
and pass-context public consumer boundaries have been removed.

## Evidence Sources

- Source intent:
  [`ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md`](../../ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md)
- Phase A2 ownership boundaries:
  [`docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`](phase_a2_residual_semantic_owner_audit.md)
- Phase B2 selected-route readiness:
  [`docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`](phase_b2_selected_route_extension_schema_readiness.md)
- Phase B2 closure:
  [`ideas/closed/201_phase_b2_selected_route_extension_schema_readiness_audit.md`](../../ideas/closed/201_phase_b2_selected_route_extension_schema_readiness_audit.md)
- Route adapter and diagnostic closures:
  [`ideas/closed/202_route3_memory_source_identity_adapter.md`](../../ideas/closed/202_route3_memory_source_identity_adapter.md),
  [`ideas/closed/203_route4_publication_identity_adapter.md`](../../ideas/closed/203_route4_publication_identity_adapter.md),
  [`ideas/closed/204_route5_edge_join_source_adapter.md`](../../ideas/closed/204_route5_edge_join_source_adapter.md),
  [`ideas/closed/205_route6_call_use_source_adapter.md`](../../ideas/closed/205_route6_call_use_source_adapter.md),
  [`ideas/closed/206_route7_comparison_provenance_diagnostic_oracle.md`](../../ideas/closed/206_route7_comparison_provenance_diagnostic_oracle.md)
- Retained aggregate, diagnostic, and residual consumer maps:
  [`docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`](prepared_function_lookups_ownership_readiness_map.md),
  [`docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`](prepared_diagnostics_oracle_replacement_plan.md),
  [`docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`](residual_route_view_consumer_migration_map.md)
- Baseline context:
  [`test_baseline.log`](../../test_baseline.log)

## Route-Specific Surface Readiness Table

| Route | Selected reader or diagnostic row | Prepared surface touched | Public consumer removal status | Retained prepared fallback/oracle | Retained target/prepared policy | Proof scope | Readiness classification | Contraction readiness result |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Route 3 memory/source | Selected Route 3 load-local memory/source adapter. Prior evidence also includes the AArch64 indirect-callee stored-value source consumer as bounded Route 3 reader history. | `memory_accesses`, `PreparedMemoryAccessLookups`, prepared load-local source helpers, same-block/global memory-source helpers, and memory/frame/stack oracle rows. | One selected semantic reader moved to Route 3 for memory/source identity only. No public prepared memory lookup group, helper family, target wrapper, printer/debug row, or oracle family was removed. | Prepared memory access and load-local fallback remains required for absent Route 3 facts, mismatches, non-memory cases, alias or address ambiguity, and target-addressing fallback. Memory/frame/stack oracles remain comparison surfaces. | Address formation, frame/global/TLS relocation, stack/frame offsets, layout, addressing-mode legality, volatile/address-space lowering policy, materialization, final operands, and policy-sensitive source-home behavior remain prepared/target-owned. | Closure proof recorded matching backend logs with 180 passed before and after, plus guarded fallback requirements from ideas 190 and 199. This proves adapter correctness and fallback preservation, not public API contraction. | retained public fallback/oracle | Not contraction-ready. Route 3 can justify future one-reader semantic migration work, but `memory_accesses` and prepared memory/source helpers stay public while target-addressing fallback and oracle consumers remain. |
| Route 4 publication | Selected AArch64 call-boundary current-block publication source reader. | Current/block-entry publication helpers, `edge_publications`, `PreparedEdgePublicationLookups`, prepared publication lookup mechanics, publication wrappers, and debug/output surfaces. | One selected semantic publication identity reader moved to Route 4. No public `edge_publications` lookup group, current/block-entry helper family, wrapper boundary, printer/debug output, or route oracle was removed. | Prepared fallback remains for absent/missing publication, mismatch, duplicate/ambiguous matches, wrong-reference handling, wrapper compatibility, and emitted/debug output preservation. | Edge-copy emission, move/home/storage policy, stack-source extension, block-order emission, immediate payload spelling, wrapper formatting, emitted strings, and publication mechanics remain prepared/target-owned. | Closure notes show semantic publication identity migrated while mechanics and output-sensitive behavior stay prepared-authoritative. Step 1 inventory records adapter evidence only, not consumer exhaustion. | retained target/prepared policy | Not contraction-ready. Route 4 publication identity is narrower than publication mechanics; `edge_publications` and prepared publication helpers must remain available for policy and compatibility surfaces. |
| Route 5 edge/join-source | `build_current_block_join_prepared_query_routing` selected as the bounded reader boundary, with indexed Route 5 current-block join-source validation through `mir::find_bir_current_block_join_source_identity`. | `PreparedEdgePublicationLookups`, `PreparedEdgePublicationSourceProducerLookups`, indexed edge-publication/source-producer helpers, current-block join-source helpers, move-bundle lookup surfaces, and edge/join wrapper oracles. | One selected current-block join-source semantic boundary moved to Route 5 when the adapter validates availability. No public edge-publication, source-producer, move-bundle, current-block join, wrapper, printer/debug, or oracle surface was removed. | Prepared fallback remains for no-source and memory-source status, duplicate/conflict rejection, absent or mismatched Route 5 facts, wrapper compatibility, branch/parallel-copy sanity, and final emitted-output stability. | Parallel-copy scheduling, source/destination homes, move bundles, branch spelling, scratch policy, cycle temporary routing, execution-site placement, storage, wrappers, final edge-copy records, and emitted output remain prepared/target-owned. | Closure proof recorded matching backend logs with 180 passed before and after. The proof establishes fail-closed Route 5 validation for one semantic boundary, not move or edge-publication contraction. | retained target/prepared policy | Not contraction-ready. Route 5 semantic edge/join-source identity does not retire move scheduling, edge-publication, wrapper, printer/debug, or oracle surfaces. |
| Route 6 call-use source | Selected AArch64 scalar call argument source-producer Route 6 adapter. | `call_plans`, `PreparedCallPlanLookups`, call argument source-producer materialization helpers, prepared call-plan fallback, and x86 `ConsumedPlans` compatibility context. | One selected call argument source identity read moved to Route 6 under strict agreement checks. No public call-plan lookup group, call argument/result helper family, x86 wrapper context, prepared call printer/debug row, or call-boundary oracle was removed. | Prepared call-plan fallback remains for absent Route 6 facts, source-id/name/value mismatch, ambiguity, invalid current-block producer pointers, scalar materialization disagreement, ABI-bound cases, x86 compatibility, and unchanged emitted output. | ABI placement, call wrapper kind, clobbers, outgoing stack sizing, byval lanes, variadic FPR counts, helper/carrier protocols, value homes, storage, movement, publication routing, aggregate transport layout, final call records, and call emission remain prepared/target-owned. | Closure notes record backend proof with 180 passed before and after and an adapter that accepts Route 6 only when route and prepared semantic source facts agree. This is one-reader adapter correctness evidence. | retained target/prepared policy | Not contraction-ready. Route 6 source identity can feed selected reads, but `call_plans` and prepared call helpers remain public authority for ABI, layout, wrapper, oracle, and output-sensitive behavior. |
| Route 7 comparison/provenance | Materialized-condition provenance reader selected as the Route 7 diagnostic/oracle surface. | Route 7 materialized-condition and fused-compare provenance helpers, prepared fused-compare and materialized-condition fallbacks, branch-policy oracles, route-debug/printer/debug rows, and string-authority surfaces. | One materialized-condition provenance reader now validates Route 7 facts and fails closed. No prepared comparison helper family, route-debug row family, printer/debug output, branch-policy oracle, or expected-string authority was removed. | Prepared fallback remains for fused compare, materialized condition, unfused fallback, absent-route cases, invalid references, duplicate facts, mismatches, route-debug/handoff compatibility, prepared printer/debug output, and branch-policy oracle behavior. | Branch targets, final branch spelling, condition suffix mapping, fused legality, hazard handling, emitted-register state, final instruction order, final assembler rows, compare/branch instruction selection, and expected strings remain prepared/AArch64 target authority. | Closure proof recorded backend proof with 180 passed before and after. The proof establishes diagnostic/oracle behavior for one provenance surface while preserving prepared/AArch64 branch authority. | diagnostic/oracle replacement prerequisite | Not contraction-ready. Route 7 provenance is a prerequisite for later route-native diagnostics, but prepared comparison, branch-policy, printer/debug, and string-authority oracles remain until equivalent route-native coverage exists. |

## Step 2 Result

No Route 3 through Route 7 surface is ready for a bounded micro-contraction idea
based on the Step 2 evidence. The selected closures prove bounded semantic
adapter correctness or diagnostic/oracle equivalence for one surface at a time.
They do not prove public consumer exhaustion, prepared fallback removal, target
policy migration, aggregate lookup retirement, or diagnostic/string-authority
replacement.

Later C2 steps should classify aggregate coupling, diagnostic authority, and
follow-up decisions without upgrading any row above from adapter greenness
alone.
