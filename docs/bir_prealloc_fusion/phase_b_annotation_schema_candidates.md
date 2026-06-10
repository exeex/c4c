# Phase B BIR Annotation Schema Candidates

Source idea: `ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md`
Status: Step 2 annotation placement classification complete.

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
fill the remaining columns with rejects, cache-only facts, bridge/oracle
fields, blockers, and follow-up idea links.

| Route | Phase A evidence source | Closed route evidence | Established BIR-owned semantic relationship | Prepared surfaces to classify later | Closure blockers or boundaries to preserve | Phase B classification status |
| --- | --- | --- | --- | --- | --- | --- |
| 1. Producer/source identity foundation | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 1, and `## Follow-Up Idea Payloads` row `NNN_bir_producer_source_identity_foundation.md`. | `ideas/closed/152_bir_producer_source_identity_foundation.md` closure note. | Same-block producer node, immediate constant, source value/name, value type, producer instruction index, materialization availability, and target-neutral producer-kind vocabulary. | `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`, `PreparedEdgePublicationSourceProducer` producer shape, `find_prepared_same_block_scalar_producer`, `evaluate_prepared_same_block_integer_constant`. | Do not import `PreparedValueHome`, register views, storage state, emitted-register availability, spill/reload behavior, operand views, frame-slot layout, final instruction order, or publication-owned enum authority. | Step 2 classified: value annotation for value/name/type and materialization availability; instruction annotation for producer node/index; per-function query index for cheap lookup. |
| 2. Select-chain and direct-global dependency identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 2, and `## Follow-Up Idea Payloads` row `NNN_bir_select_chain_direct_global_identity.md`. | `ideas/closed/153_bir_select_chain_direct_global_identity.md` closure notes. | Select-chain source producer, root value/name, root-is-select flag, root instruction index, direct `LoadGlobalInst` dependency, scalar materialization eligibility, and combined identity lookup. | `PreparedDirectGlobalSelectChainDependency`, `PreparedSelectChainDependencyQuery`, `PreparedScalarSelectChainMaterialization`, `find_prepared_select_chain_source_producer`, `find_prepared_direct_global_select_chain_dependency`, `find_prepared_scalar_select_chain_materialization`. | Do not include target materialization cost, hazard decisions, register availability, publication routing policy, call ABI behavior, or final AArch64 move/branch choices. | Step 2 classified: value annotation for select-root identity and scalar eligibility; instruction annotation for root instruction and direct `LoadGlobalInst` dependency; per-function index for before-instruction lookup. |
| 3. Memory/access semantic identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 3, and `## Follow-Up Idea Payloads` row `NNN_bir_memory_access_identity.md`. | `ideas/closed/154_bir_memory_access_identity.md` closure notes. | Direct memory access identity, same-block global-load identity, same-block load-local source identity, result/stored value identity, address-space/volatile bits, semantic base kind, and local/global/pointer/string source identity where representable. | `PreparedAddress`, `PreparedMemoryAccess`, `PreparedSameBlockGlobalLoadAccess`, `PreparedSameBlockLoadLocalStoredValueSource`, `find_prepared_same_block_load_local_source_producer`, same-block global-load lookup helpers. | Do not import frame slot ids, concrete offsets, size/align layout, base-plus-offset legality, TLS register or relocation spelling, GOT/direct/page-low policy, target addressing-mode choice, or AArch64 memory operand legality. | Step 2 classified: instruction annotation on load/store/address-producing nodes for access identity; value annotation for result/stored-value links; per-function index for same-block global-load and load-local source lookup. |
| 4. Block-entry and current-block publication identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 4, and `## Follow-Up Idea Payloads` row `NNN_bir_block_entry_publication_identity.md`. | `ideas/closed/155_bir_block_entry_publication_identity.md` closure note. | Current-block and block-entry value availability, source producer identity, produced BIR value/name, producer instruction/index, and source-producer kind. | `PreparedCurrentBlockPublicationConsumption`, current-block entry publication lookup helpers, scalar publication plan reads that ask which semantic source is available. | Do not import hook kind, destination home, storage encoding, stack-source extension policy, register view conversion, immediate publication payloads, emitted storage availability, or scalar publication emission policy. | Step 2 classified: block annotation for entry/current-block availability keyed by value and program point; value annotation for produced value/source identity; cache-only treatment for publication planning mechanics. |
| 5. CFG edge publication and join-source identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 5, and `## Follow-Up Idea Payloads` row `NNN_bir_cfg_edge_publication_identity.md`. | `ideas/closed/156_bir_cfg_edge_publication_identity.md` closure note. | CFG edge publication identity keyed by predecessor, successor, and destination value/name; source BIR value/name/kind; source producer block/instruction; optional memory-source identity; current-block join incoming-expression/source identity. | `PreparedEdgePublication`, `PreparedEdgePublicationLookups`, `PreparedEdgeCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceFact(s)`, edge publication source producer lookups. | Do not import move bundle or step order, cycle temporary routing, execution site/block placement, phase/carrier policy, coalescing or redundancy flags, destination register names, storage-sharing checks, or prepared move records. | Step 2 classified: edge annotation for predecessor/successor/destination/source identity; block annotation for current-block join source facts; move and parallel-copy execution data remain outside BIR. |
| 6. Call-boundary semantic source facts | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 6, and `## Follow-Up Idea Payloads` row `NNN_bir_call_boundary_source_facts.md`. | `ideas/closed/157_bir_call_boundary_source_facts.md` closure note. | Call argument/result source identity for the source-producer and direct-global dependency families, including source value/base value/name, same-block materializable producer, direct global select-chain dependency, and semantic links that do not require ABI/layout authority. | `PreparedCallArgumentSourceSelection`, `PreparedCallArgumentSourceProducerMaterialization`, `PreparedCallArgumentDirectGlobalSelectChainDependency`, `PreparedCallArgumentPublicationSourceRouting`, call argument/result reads that only need semantic source facts. | Do not import ABI register or stack placement, outgoing stack sizing, variadic FPR count, preservation/clobber sets, byval aggregate lanes, scratch requirements, destination homes, helper/carrier protocols, aggregate transport lane layout, or ABI/layout-bound publication-routing source-selection reads. | Step 2 classified: instruction annotation on `CallInst` argument/result uses for source facts; value annotation for result source values; function annotation only for call-site lookup/indexing, not ABI placement. |
| 7. Comparison and materialized-condition producer identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 7, and `## Follow-Up Idea Payloads` row `NNN_bir_comparison_condition_producer_identity.md`. | `ideas/closed/158_bir_comparison_condition_producer_identity.md` closure notes. | Comparison producer identity keyed by branch condition or condition value; lhs/rhs producer nodes or integer constants; comparison-producing `BinaryInst`; condition value name; producer instruction index; fused-operand and materialized-condition provenance. | `PreparedFusedCompareOperandProducer`, `PreparedFusedCompareOperandProducerFacts`, `PreparedMaterializedConditionProducer`, `find_prepared_fused_compare_operand_producer_facts`, `find_prepared_materialized_condition_producer`. | Do not import fused-compare legality, condition-code selection, branch emission strategy, final instruction records/errors, hazard handling, emitted-register state, or AArch64 compare/branch instruction selection. | Step 2 classified: instruction annotation for compare/condition producer provenance; terminator annotation for branch-condition provenance; no branch lowering policy in BIR. |

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

## Candidate BIR Annotation Schema Map

This Step 2 map classifies the Phase A route facts by the BIR location that
should own each durable annotation. Placement here means "where the semantic
fact should be stored or indexed if it becomes a future schema", not an
implementation commitment in this analysis-only plan.

### Value Annotation Candidates

| Candidate | Traceability | Proposed fields | Placement rationale |
| --- | --- | --- | --- |
| Value producer/materialization identity | Route 1 in `phase_a_normalization_candidates.md`; `ideas/closed/152_bir_producer_source_identity_foundation.md`. | Value/name/type identity, target-neutral producer kind, immediate constant when present, materialization availability, and optional producer instruction reference. | The queried subject is a BIR value at a program point. Value placement keeps value/name/type and materialization facts close to consumers while instruction references preserve the producing node without importing homes or storage. |
| Select-root value identity and scalar eligibility | Route 2 in `phase_a_normalization_candidates.md`; `ideas/closed/153_bir_select_chain_direct_global_identity.md`. | Root value/name, root-is-select, scalar materialization eligibility, and direct-global dependency summary. | Select-chain queries start from a value root. Value placement is appropriate for root identity and eligibility; the instruction-level map owns the concrete root producer and direct `LoadGlobalInst` link. |
| Memory result or stored-value semantic link | Route 3 in `phase_a_normalization_candidates.md`; `ideas/closed/154_bir_memory_access_identity.md`. | Load result value, store source value, local/global/pointer/string source identity, and same-block load-local stored-value source link. | The semantic relation answers which BIR value is read, produced, or stored. Value placement avoids copying `PreparedAddress` while still giving memory and store-source consumers stable value identity. |
| Produced publication value/source identity | Route 4 in `phase_a_normalization_candidates.md`; `ideas/closed/155_bir_block_entry_publication_identity.md`. | Produced BIR value/name, consumed value/name, source producer kind, source producer value/name. | Publication availability is block-scoped, but the stable payload is a value relationship. Value placement should hold the value/source identity while block placement owns availability by block and program point. |
| Call result source identity | Route 6 in `phase_a_normalization_candidates.md`; `ideas/closed/157_bir_call_boundary_source_facts.md`. | Result value/name, source value/base value/name, materializable same-block producer, direct-global dependency reference. | Result-source facts are value provenance facts for a `CallInst` result. The call instruction owns the call-site use map; result values may carry the result-side provenance for consumers that start from the produced value. |

### Instruction Annotation Candidates

| Candidate | Traceability | Proposed fields | Placement rationale |
| --- | --- | --- | --- |
| Same-block producer node record | Route 1 in `phase_a_normalization_candidates.md`; `ideas/closed/152_bir_producer_source_identity_foundation.md`. | Producer instruction id/index, producer kind, produced value/name/type, immediate constant when the producer is constant-like. | The producer is an instruction-local fact over an ordered BIR block. Instruction placement makes the producing node authoritative and leaves value placement to expose query-friendly provenance. |
| Select-chain and direct-global producer dependency | Route 2 in `phase_a_normalization_candidates.md`; `ideas/closed/153_bir_select_chain_direct_global_identity.md`. | Root instruction index, source producer instruction, direct `LoadGlobalInst` dependency, combined identity lookup key. | The root and direct-global dependency are producer-instruction facts. Instruction placement prevents these facts from becoming call-routing or publication-policy shortcuts. |
| Memory access semantic identity | Route 3 in `phase_a_normalization_candidates.md`; `ideas/closed/154_bir_memory_access_identity.md`. | Load/store/address node id, result or stored value, address-space, volatile flag, semantic base kind, source slot/global/pointer/string identity. | Memory identity attaches naturally to load, store, and address-producing instructions. This placement admits semantic access facts while excluding frame offsets, relocations, and target addressing legality. |
| Call-use source facts | Route 6 in `phase_a_normalization_candidates.md`; `ideas/closed/157_bir_call_boundary_source_facts.md`. | Per-argument and per-result source value/base value/name, same-block producer link, direct-global select-chain dependency, memory/publication source link where semantic. | Call-boundary provenance is attached to a specific `CallInst` use/result position. Instruction placement avoids function-wide ABI data and keeps register/stack placement outside BIR. |
| Comparison or materialized-condition producer provenance | Route 7 in `phase_a_normalization_candidates.md`; `ideas/closed/158_bir_comparison_condition_producer_identity.md`. | Comparison-producing `BinaryInst`, condition value name, lhs/rhs producer nodes or integer constants, producer instruction index. | The comparable operands and materialized condition are instruction-provenance facts. Instruction placement gives comparison consumers stable provenance without owning fused-compare legality or branch emission. |

### Terminator Annotation Candidates

| Candidate | Traceability | Proposed fields | Placement rationale |
| --- | --- | --- | --- |
| Branch-condition producer provenance | Route 7 in `phase_a_normalization_candidates.md`; `ideas/closed/158_bir_comparison_condition_producer_identity.md`. | Branch condition value, comparison-producing instruction reference, lhs/rhs producer identity or constants, materialized-condition producer reference. | A conditional terminator is the consumer of the branch condition. Terminator placement lets branch consumers ask for condition provenance without making BIR responsible for condition-code selection, compare/branch fusion, or final target branch spelling. |

### Block Annotation Candidates

| Candidate | Traceability | Proposed fields | Placement rationale |
| --- | --- | --- | --- |
| Block-entry and current-block value availability | Route 4 in `phase_a_normalization_candidates.md`; `ideas/closed/155_bir_block_entry_publication_identity.md`. | Block label, value/name, before-instruction index, source producer identity, produced value/name, source-producer kind. | Availability is scoped by block and program point. Block placement owns "available at entry/current point" while value placement owns the underlying produced/source value identity. |
| Current-block join incoming source identity | Route 5 in `phase_a_normalization_candidates.md`; `ideas/closed/156_bir_cfg_edge_publication_identity.md`. | Join block label, incoming expression/value, predecessor label where known, source value/name/kind, optional memory-source identity. | Join-source facts are about a block's incoming value interpretation before move scheduling. Block placement keeps the relationship in BIR CFG space without importing parallel-copy execution order or storage-sharing decisions. |

### Edge Annotation Candidates

| Candidate | Traceability | Proposed fields | Placement rationale |
| --- | --- | --- | --- |
| CFG edge publication source identity | Route 5 in `phase_a_normalization_candidates.md`; `ideas/closed/156_bir_cfg_edge_publication_identity.md`. | Predecessor label, successor label, destination value/name, source value/name/kind, source producer block/instruction, optional memory-source identity, no-source marker. | The relationship is keyed by a CFG edge and a destination value. Edge placement preserves predecessor/successor identity and prevents the schema from absorbing move-bundle order, cycle temporaries, execution-site policy, or destination registers. |

### Function Annotation Candidates

| Candidate | Traceability | Proposed fields | Placement rationale |
| --- | --- | --- | --- |
| Per-function relationship indexes for cheap lookup | Routes 1 through 7 in `phase_a_normalization_candidates.md`; closure notes `152` through `158`. | Function-local indexes keyed by stable BIR ids: block label, instruction index, value/name, call-site position, edge key, and relationship kind. | Several accepted facts are queried by `{function, block, value, before-instruction}` rather than by direct node traversal. Function placement should own indexes or caches that accelerate lookup while the durable payload remains attached to value, instruction, terminator, block, or edge annotations. |
| Call-site semantic source lookup table | Route 6 in `phase_a_normalization_candidates.md`; `ideas/closed/157_bir_call_boundary_source_facts.md`. | Function-local map from `CallInst` id and argument/result ordinal to the instruction/value annotation records above. | This is function-scoped only as an index. The call instruction remains the semantic owner; the function-level table prevents expensive scans and must not grow into ABI placement, outgoing stack, variadic, clobber, scratch, helper, or aggregate-lane metadata. |

### Module-Level Annotation Candidates

| Candidate | Traceability | Proposed fields | Placement rationale |
| --- | --- | --- | --- |
| No accepted Phase A route fact requires durable module-level lowering metadata. | Routes 1 through 7 in `phase_a_normalization_candidates.md`; closure notes `152` through `158`. | None for Step 2. Module-level name tables and target profile may be referenced by ids, but the Phase A semantic relationships are function, block, edge, instruction, terminator, or value scoped. | A module-level placement would invite target profile, ABI, relocation, global addressing, or lowering-policy leakage. Step 2 therefore treats module-level storage as rejected for durable schema candidates unless a later phase identifies a genuinely module-scoped, target-neutral relationship. |

## Placement Summary By Phase A Route

| Route | Primary placement | Secondary placement | Rationale |
| --- | --- | --- | --- |
| 1. Producer/source identity foundation | Value and instruction annotations. | Function-local lookup index. | The source fact is a value-to-producer relationship at a block position. Value and instruction payloads own the relationship; function indexing keeps prepared-query lookup shape cheap. |
| 2. Select-chain and direct-global dependency identity | Value and instruction annotations. | Function-local lookup index. | Root and eligibility are value facts; direct `LoadGlobalInst` and root instruction identity are instruction facts. |
| 3. Memory/access semantic identity | Instruction annotations. | Value annotations and function-local lookup index. | Load/store/address nodes own access identity; values carry result/stored links; function indexes support same-block global-load and load-local queries. |
| 4. Block-entry and current-block publication identity | Block annotations. | Value annotations. | Availability is block/program-point scoped, while the payload names produced/source values. |
| 5. CFG edge publication and join-source identity | Edge annotations. | Block annotations. | Publication source identity is edge-keyed; current-block join interpretation is block-keyed. |
| 6. Call-boundary semantic source facts | Instruction annotations on `CallInst`. | Value annotations and function-local lookup index. | Call argument/result facts are use-position facts on a call instruction; result values may expose provenance; indexes avoid scanning. |
| 7. Comparison and materialized-condition producer identity | Instruction annotations. | Terminator annotations. | Compare/materialized-condition provenance belongs to producer instructions; branch-condition provenance belongs to the consuming terminator. |
