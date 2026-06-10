# Phase B BIR Annotation Schema Candidates

Source idea: `ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md`
Status: Step 1 evidence index started.

This artifact is the durable Phase B analysis surface for deciding which
`Prepared*` facts should become BIR-owned annotations after Phase A established
the target-neutral semantic relationships. Phase B is analysis-only: this file
classifies fields and records follow-up schema ideas, but does not change
compiler implementation or schema code.

## Provenance

Primary Phase A evidence:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `ideas/closed/151_phase_a_bir_normalization_candidate_audit.md`

Closed Phase A route follow-ups:

- `ideas/closed/152_bir_producer_source_identity_foundation.md`
- `ideas/closed/153_bir_select_chain_direct_global_identity.md`
- `ideas/closed/154_bir_memory_access_identity.md`
- `ideas/closed/155_bir_block_entry_publication_identity.md`
- `ideas/closed/156_bir_cfg_edge_publication_identity.md`
- `ideas/closed/157_bir_call_boundary_source_facts.md`
- `ideas/closed/158_bir_comparison_condition_producer_identity.md`

Phase B should treat these sources as the allowed source of truth for schema
decisions. New annotation candidates must trace back to a Phase A route,
candidate row, dependency-order row, follow-up payload, or closure note.

## Evidence Source List

| Source | Evidence role for Phase B |
| --- | --- |
| `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` | Defines the seven BIR-normalization route groups, accepted semantic subsets, rejected non-BIR fact families, dependency order, switch timing rule, and future proof expectations. |
| `ideas/closed/151_phase_a_bir_normalization_candidate_audit.md` | Confirms Phase A closure, names the seven route groups, summarizes reject boundaries, and records that the Phase A artifact is the durable handoff for later phases. |
| `ideas/closed/152_bir_producer_source_identity_foundation.md` | Closure evidence for the producer/source foundation: BIR-owned producer-kind vocabulary and query records exist for same-block producers, immediate constants, value/name/type identity, producer instruction index, and materialization availability. |
| `ideas/closed/153_bir_select_chain_direct_global_identity.md` | Closure evidence for select-chain identity: BIR-owned records and query APIs cover source producer, direct-global dependency, scalar materialization eligibility, and combined identity lookup. |
| `ideas/closed/154_bir_memory_access_identity.md` | Closure evidence for memory identity: BIR-owned direct memory access, same-block global-load identity, and same-block load-local source identity were added; target-layout/addressing facts remain outside BIR. |
| `ideas/closed/155_bir_block_entry_publication_identity.md` | Closure evidence for current-block publication identity: BIR-owned semantic publication queries cover current-block and block-entry availability while prepared publication mechanics remain downstream. |
| `ideas/closed/156_bir_cfg_edge_publication_identity.md` | Closure evidence for CFG edge identity: BIR edge queries cover normal edge, join-source, optional memory-source, and no-source semantic identities while parallel-copy execution remains outside BIR. |
| `ideas/closed/157_bir_call_boundary_source_facts.md` | Closure evidence for call-boundary source facts: production BIR call-boundary semantic source facts cover source-producer and direct-global dependency families; ABI/layout publication-routing reads remain outside scope. |
| `ideas/closed/158_bir_comparison_condition_producer_identity.md` | Closure evidence for comparison identity: BIR comparison producer queries are proven against prepared fused-operand and materialized-condition producer answers without moving target branch policy into BIR. |

## Phase A Route Coverage Table

This table is the Step 1 route coverage skeleton. Later Phase B steps should
fill the classification columns with schema candidates, rejects, cache-only
facts, bridge/oracle fields, blockers, and follow-up idea links.

| Route | Phase A evidence source | Closed route evidence | Established BIR-owned semantic relationship | Prepared surfaces to classify later | Closure blockers or boundaries to preserve | Phase B classification status |
| --- | --- | --- | --- | --- | --- | --- |
| 1. Producer/source identity foundation | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 1, and `## Follow-Up Idea Payloads` row `NNN_bir_producer_source_identity_foundation.md`. | `ideas/closed/152_bir_producer_source_identity_foundation.md` closure note. | Same-block producer node, immediate constant, source value/name, value type, producer instruction index, materialization availability, and target-neutral producer-kind vocabulary. | `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`, `PreparedEdgePublicationSourceProducer` producer shape, `find_prepared_same_block_scalar_producer`, `evaluate_prepared_same_block_integer_constant`. | Do not import `PreparedValueHome`, register views, storage state, emitted-register availability, spill/reload behavior, operand views, frame-slot layout, final instruction order, or publication-owned enum authority. | Step 2 pending: decide value or instruction annotation placement versus private query-cache placement. |
| 2. Select-chain and direct-global dependency identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 2, and `## Follow-Up Idea Payloads` row `NNN_bir_select_chain_direct_global_identity.md`. | `ideas/closed/153_bir_select_chain_direct_global_identity.md` closure notes. | Select-chain source producer, root value/name, root-is-select flag, root instruction index, direct `LoadGlobalInst` dependency, scalar materialization eligibility, and combined identity lookup. | `PreparedDirectGlobalSelectChainDependency`, `PreparedSelectChainDependencyQuery`, `PreparedScalarSelectChainMaterialization`, `find_prepared_select_chain_source_producer`, `find_prepared_direct_global_select_chain_dependency`, `find_prepared_scalar_select_chain_materialization`. | Do not include target materialization cost, hazard decisions, register availability, publication routing policy, call ABI behavior, or final AArch64 move/branch choices. | Step 2 pending: decide value/instruction annotation placement for select roots and direct-global dependencies. |
| 3. Memory/access semantic identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 3, and `## Follow-Up Idea Payloads` row `NNN_bir_memory_access_identity.md`. | `ideas/closed/154_bir_memory_access_identity.md` closure notes. | Direct memory access identity, same-block global-load identity, same-block load-local source identity, result/stored value identity, address-space/volatile bits, semantic base kind, and local/global/pointer/string source identity where representable. | `PreparedAddress`, `PreparedMemoryAccess`, `PreparedSameBlockGlobalLoadAccess`, `PreparedSameBlockLoadLocalStoredValueSource`, `find_prepared_same_block_load_local_source_producer`, same-block global-load lookup helpers. | Do not import frame slot ids, concrete offsets, size/align layout, base-plus-offset legality, TLS register or relocation spelling, GOT/direct/page-low policy, target addressing-mode choice, or AArch64 memory operand legality. | Step 2 pending: decide memory instruction annotation placement and which address facts remain target-facing. |
| 4. Block-entry and current-block publication identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 4, and `## Follow-Up Idea Payloads` row `NNN_bir_block_entry_publication_identity.md`. | `ideas/closed/155_bir_block_entry_publication_identity.md` closure note. | Current-block and block-entry value availability, source producer identity, produced BIR value/name, producer instruction/index, and source-producer kind. | `PreparedCurrentBlockPublicationConsumption`, current-block entry publication lookup helpers, scalar publication plan reads that ask which semantic source is available. | Do not import hook kind, destination home, storage encoding, stack-source extension policy, register view conversion, immediate publication payloads, emitted storage availability, or scalar publication emission policy. | Step 2 pending: decide block/value annotation placement for availability facts and cache-only treatment for planning mechanics. |
| 5. CFG edge publication and join-source identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 5, and `## Follow-Up Idea Payloads` row `NNN_bir_cfg_edge_publication_identity.md`. | `ideas/closed/156_bir_cfg_edge_publication_identity.md` closure note. | CFG edge publication identity keyed by predecessor, successor, and destination value/name; source BIR value/name/kind; source producer block/instruction; optional memory-source identity; current-block join incoming-expression/source identity. | `PreparedEdgePublication`, `PreparedEdgePublicationLookups`, `PreparedEdgeCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceFact(s)`, edge publication source producer lookups. | Do not import move bundle or step order, cycle temporary routing, execution site/block placement, phase/carrier policy, coalescing or redundancy flags, destination register names, storage-sharing checks, or prepared move records. | Step 2 pending: decide edge annotation placement and reject parallel-copy execution data. |
| 6. Call-boundary semantic source facts | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 6, and `## Follow-Up Idea Payloads` row `NNN_bir_call_boundary_source_facts.md`. | `ideas/closed/157_bir_call_boundary_source_facts.md` closure note. | Call argument/result source identity for the source-producer and direct-global dependency families, including source value/base value/name, same-block materializable producer, direct global select-chain dependency, and semantic links that do not require ABI/layout authority. | `PreparedCallArgumentSourceSelection`, `PreparedCallArgumentSourceProducerMaterialization`, `PreparedCallArgumentDirectGlobalSelectChainDependency`, `PreparedCallArgumentPublicationSourceRouting`, call argument/result reads that only need semantic source facts. | Do not import ABI register or stack placement, outgoing stack sizing, variadic FPR count, preservation/clobber sets, byval aggregate lanes, scratch requirements, destination homes, helper/carrier protocols, aggregate transport lane layout, or ABI/layout-bound publication-routing source-selection reads. | Step 2 pending: decide call instruction/function annotation placement and classify remaining call-plan fields as target-facing or cache-only. |
| 7. Comparison and materialized-condition producer identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 7, and `## Follow-Up Idea Payloads` row `NNN_bir_comparison_condition_producer_identity.md`. | `ideas/closed/158_bir_comparison_condition_producer_identity.md` closure notes. | Comparison producer identity keyed by branch condition or condition value; lhs/rhs producer nodes or integer constants; comparison-producing `BinaryInst`; condition value name; producer instruction index; fused-operand and materialized-condition provenance. | `PreparedFusedCompareOperandProducer`, `PreparedFusedCompareOperandProducerFacts`, `PreparedMaterializedConditionProducer`, `find_prepared_fused_compare_operand_producer_facts`, `find_prepared_materialized_condition_producer`. | Do not import fused-compare legality, condition-code selection, branch emission strategy, final instruction records/errors, hazard handling, emitted-register state, or AArch64 compare/branch instruction selection. | Step 2 pending: decide instruction or terminator annotation placement for comparison provenance. |

## Phase A Boundaries For Later Classification

The Phase A artifact establishes a positive schema boundary and a negative
target-facing boundary:

- BIR schemas may record stable BIR ids, value/name/type identity, block/edge
  ids, producer instruction indexes, semantic base/source kinds,
  address-space/volatile bits, direct global-load dependency, and integer
  constants.
- BIR schemas must not record homes, stack offsets, frame slots, physical
  registers, ABI placements, target addressing legality, relocation spelling,
  parallel-copy execution policy, storage hooks, scratch resources, or final
  instruction records.

Step 2 should use the route coverage rows above to decide candidate placement
across value, instruction, terminator, block, edge, function, and module-level
annotation locations.
