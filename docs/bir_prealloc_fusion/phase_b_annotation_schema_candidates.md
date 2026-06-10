# Phase B BIR Annotation Schema Candidates

Source idea: `ideas/open/152_phase_b_bir_annotation_schema_candidate_audit.md`
Status: Finalized for lifecycle closure review.

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

This table started as the Step 1 route coverage skeleton. Through Step 3 it
records the accepted placement decision for each route and points later sections
at the reject, cache-only, bridge/oracle, target-facing, and blocker boundary.

| Route | Phase A evidence source | Closed route evidence | Established BIR-owned semantic relationship | Prepared surfaces to classify later | Closure blockers or boundaries to preserve | Phase B classification status |
| --- | --- | --- | --- | --- | --- | --- |
| 1. Producer/source identity foundation | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 1, and `## Follow-Up Idea Payloads` row `NNN_bir_producer_source_identity_foundation.md`. | `ideas/closed/152_bir_producer_source_identity_foundation.md` closure note. | Same-block producer node, immediate constant, source value/name, value type, producer instruction index, materialization availability, and target-neutral producer-kind vocabulary. | `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`, `PreparedEdgePublicationSourceProducer` producer shape, `find_prepared_same_block_scalar_producer`, `evaluate_prepared_same_block_integer_constant`. | Do not import `PreparedValueHome`, register views, storage state, emitted-register availability, spill/reload behavior, operand views, frame-slot layout, final instruction order, or publication-owned enum authority. | Step 3 classified: accepted value/instruction annotations plus function index; non-schema boundary summarized below. |
| 2. Select-chain and direct-global dependency identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 2, and `## Follow-Up Idea Payloads` row `NNN_bir_select_chain_direct_global_identity.md`. | `ideas/closed/153_bir_select_chain_direct_global_identity.md` closure notes. | Select-chain source producer, root value/name, root-is-select flag, root instruction index, direct `LoadGlobalInst` dependency, scalar materialization eligibility, and combined identity lookup. | `PreparedDirectGlobalSelectChainDependency`, `PreparedSelectChainDependencyQuery`, `PreparedScalarSelectChainMaterialization`, `find_prepared_select_chain_source_producer`, `find_prepared_direct_global_select_chain_dependency`, `find_prepared_scalar_select_chain_materialization`. | Do not include target materialization cost, hazard decisions, register availability, publication routing policy, call ABI behavior, or final AArch64 move/branch choices. | Step 3 classified: accepted value/instruction annotations plus function index; non-schema boundary summarized below. |
| 3. Memory/access semantic identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 3, and `## Follow-Up Idea Payloads` row `NNN_bir_memory_access_identity.md`. | `ideas/closed/154_bir_memory_access_identity.md` closure notes. | Direct memory access identity, same-block global-load identity, same-block load-local source identity, result/stored value identity, address-space/volatile bits, semantic base kind, and local/global/pointer/string source identity where representable. | `PreparedAddress`, `PreparedMemoryAccess`, `PreparedSameBlockGlobalLoadAccess`, `PreparedSameBlockLoadLocalStoredValueSource`, `find_prepared_same_block_load_local_source_producer`, same-block global-load lookup helpers. | Do not import frame slot ids, concrete offsets, size/align layout, base-plus-offset legality, TLS register or relocation spelling, GOT/direct/page-low policy, target addressing-mode choice, or AArch64 memory operand legality. | Step 3 classified: accepted instruction/value annotations plus function index; non-schema boundary summarized below. |
| 4. Block-entry and current-block publication identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 4, and `## Follow-Up Idea Payloads` row `NNN_bir_block_entry_publication_identity.md`. | `ideas/closed/155_bir_block_entry_publication_identity.md` closure note. | Current-block and block-entry value availability, source producer identity, produced BIR value/name, producer instruction/index, and source-producer kind. | `PreparedCurrentBlockPublicationConsumption`, current-block entry publication lookup helpers, scalar publication plan reads that ask which semantic source is available. | Do not import hook kind, destination home, storage encoding, stack-source extension policy, register view conversion, immediate publication payloads, emitted storage availability, or scalar publication emission policy. | Step 3 classified: accepted block/value annotations; publication planning mechanics rejected or cache-only. |
| 5. CFG edge publication and join-source identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 5, and `## Follow-Up Idea Payloads` row `NNN_bir_cfg_edge_publication_identity.md`. | `ideas/closed/156_bir_cfg_edge_publication_identity.md` closure note. | CFG edge publication identity keyed by predecessor, successor, and destination value/name; source BIR value/name/kind; source producer block/instruction; optional memory-source identity; current-block join incoming-expression/source identity. | `PreparedEdgePublication`, `PreparedEdgePublicationLookups`, `PreparedEdgeCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceFact(s)`, edge publication source producer lookups. | Do not import move bundle or step order, cycle temporary routing, execution site/block placement, phase/carrier policy, coalescing or redundancy flags, destination register names, storage-sharing checks, or prepared move records. | Step 3 classified: accepted edge/block annotations; move and parallel-copy execution data rejected or cache-only. |
| 6. Call-boundary semantic source facts | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 6, and `## Follow-Up Idea Payloads` row `NNN_bir_call_boundary_source_facts.md`. | `ideas/closed/157_bir_call_boundary_source_facts.md` closure note. | Call argument/result source identity for the source-producer and direct-global dependency families, including source value/base value/name, same-block materializable producer, direct global select-chain dependency, and semantic links that do not require ABI/layout authority. | `PreparedCallArgumentSourceSelection`, `PreparedCallArgumentSourceProducerMaterialization`, `PreparedCallArgumentDirectGlobalSelectChainDependency`, `PreparedCallArgumentPublicationSourceRouting`, call argument/result reads that only need semantic source facts. | Do not import ABI register or stack placement, outgoing stack sizing, variadic FPR count, preservation/clobber sets, byval aggregate lanes, scratch requirements, destination homes, helper/carrier protocols, aggregate transport lane layout, or ABI/layout-bound publication-routing source-selection reads. | Step 3 classified: accepted call instruction/value annotations plus function index; ABI/layout call facts rejected or left as oracles. |
| 7. Comparison and materialized-condition producer identity | `phase_a_normalization_candidates.md` `## BIR-Normalization Candidates`, `## Dependency Order` row 7, and `## Follow-Up Idea Payloads` row `NNN_bir_comparison_condition_producer_identity.md`. | `ideas/closed/158_bir_comparison_condition_producer_identity.md` closure notes. | Comparison producer identity keyed by branch condition or condition value; lhs/rhs producer nodes or integer constants; comparison-producing `BinaryInst`; condition value name; producer instruction index; fused-operand and materialized-condition provenance. | `PreparedFusedCompareOperandProducer`, `PreparedFusedCompareOperandProducerFacts`, `PreparedMaterializedConditionProducer`, `find_prepared_fused_compare_operand_producer_facts`, `find_prepared_materialized_condition_producer`. | Do not import fused-compare legality, condition-code selection, branch emission strategy, final instruction records/errors, hazard handling, emitted-register state, or AArch64 compare/branch instruction selection. | Step 3 classified: accepted instruction/terminator annotations; branch lowering policy rejected. |

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

## Non-Schema Classification Map

Step 3 separates the accepted schema candidates above from data that should not
become durable BIR annotation payload. The boundary is field-based, not
struct-based: BIR may own the semantic identity subset proven by Phase A, but
must not copy whole `Prepared*` structs, `Prepared*Plan` records,
`Prepared*Publication` records, `PreparedAddress`, or `PreparedValueHome`
shapes into BIR.

### Whole Prepared Struct Copy Rejects

| Prepared surface | Classification | Reason |
| --- | --- | --- |
| `PreparedBirModule` | Reject as schema shape. | It aggregates raw BIR plus target profile, names, control flow, value locations, stack layout, addressing, liveness, regalloc, frame/dynamic-stack/call/store-source/variadic/storage/special-carrier/runtime-helper plans, phase state, and notes. That is a prealloc universe, not a BIR annotation schema. |
| `PreparedFunctionLookups` | Private aggregate/cache facade. | It is a one-pass lookup bundle for many domains. Future BIR schemas may provide typed relationship queries, but the aggregate facade should remain an implementation detail while migration keeps cheap lookup identity. |
| `PreparedValueHome`, `PreparedStoragePlanValue`, `PreparedRegisterPlacement`, `PreparedFrameSlot`, and related value-location/storage records. | Reject. | These encode allocation, storage, frame, stack, and register placement. Phase A explicitly leaves homes, frame slots, physical registers, storage encoding, and emitted-register availability outside BIR. |
| `PreparedScalarPublicationPlan`, `PreparedStoreSourcePublicationPlan`, `PreparedFormalPublicationPlan`, `PreparedTypedStackSourcePublication`, and `PreparedAggregateStackSourceAuthority`. | Mixed surface; reject wholesale copying. | Only source/value/access identity may feed accepted annotations. Hook kind, destination home, stack-source extension, register view conversion, immediate payload spelling, stack offsets, size/align, and emission policy remain private or target-facing. |
| `PreparedEdgePublication`, prepared move bundles, prepared move records, and parallel-copy execution records. | Mixed surface; reject wholesale copying. | Edge source identity is a schema candidate, but move order, cycle temporary routing, execution site, carrier/phase policy, storage sharing, destination registers, and redundancy/coalescing flags are copy-scheduling policy. |
| `PreparedCallArgumentPlan`, call result/publication plans, outgoing-stack plans, and aggregate transport lane records. | Mixed surface; reject wholesale copying. | Call source facts are schema candidates only where they describe BIR source identity. ABI registers, stack placement, outgoing stack sizing, variadic FPR count, preservation/clobber sets, byval lanes, scratch, helper/carrier protocols, and aggregate transport layout stay outside BIR. |
| `PreparedAddress` and address materialization records. | Mixed surface; reject wholesale copying. | BIR can own semantic memory/access identity, but frame slot ids, byte offsets, size/align, `can_use_base_plus_offset`, TLS register/relocation spelling, global materialization policy, and AArch64 addressing-mode legality remain target/layout facts. |
| Final prepared instruction records and record-error families. | Reject. | Branch/compare fusion legality, condition-code selection, memory operand record formation, spill/reload pseudo selection, retargeting, record errors, and final instruction order are MIR/codegen behavior. |

### Private Or Cache-Only Data

| Data family | Classification | Notes |
| --- | --- | --- |
| Function-local lookup indexes over accepted relationships. | Cache-only/index when the durable payload already lives on value, instruction, terminator, block, or edge annotations. | The index may be BIR-owned for query cost, but it should point at typed annotation records rather than duplicate semantic payload or become a new lowering plan. |
| `PreparedFunctionLookups`-style aggregate maps for call plans, memory accesses, move bundles, return chains, edge publications, and source producers. | Private prealloc cache/facade. | These preserve migration performance and compatibility while consumers switch one relationship at a time. They are not durable schema surfaces. |
| Current-block publication availability caches used during planning. | Cache-only except for the accepted block/value source relationship. | Availability by block/value/program point is schema material; reuse flags, emitted-storage availability, and register-view availability are planning caches. |
| Edge-publication source-producer lookup tables. | Cache-only/index over edge annotations. | The durable candidate is the predecessor/successor/destination/source identity. Lookup tables should be rebuildable from edge/block/value relationship annotations. |
| Same-block/select-chain/memory/call/comparison prepared query helpers. | Private query implementation or migration bridge. | Helpers may compare BIR answers with prepared answers during migration, but the helper API shape is not itself schema. |
| Return-chain helper facts not tied to a Phase A accepted semantic route. | Cache/private until separately justified. | The source idea requires inspection of return-chain facts, but Phase A did not establish a standalone return-chain annotation route in the seven accepted groups. |

### Bridge And Oracle Data

| Bridge/oracle surface | Role during migration | Durable schema boundary |
| --- | --- | --- |
| Existing prepared same-block producer and integer-constant queries. | Oracle for Route 1 BIR producer/materialization answers. | Durable BIR may store producer kind, source value/name/type, producer instruction/index, immediate constants, and materialization availability; the old prepared query remains a comparison fallback. |
| Existing select-chain and direct-global dependency queries. | Oracle for Route 2 select-root, direct-global, scalar eligibility, and negative-case equivalence. | BIR should store target-neutral select/global dependency identity, not call routing, materialization cost, hazard, or register availability policy. |
| Existing memory/access prepared queries. | Oracle for Route 3 same-block global-load, load-local stored-value source, volatile/address-space, and base-kind equivalence. | BIR owns memory/source identity only; prepared layout/addressing answers remain downstream. |
| Existing current-block publication reads. | Oracle for Route 4 available/unavailable source/value/producer identity. | BIR owns semantic availability, not storage hooks, register views, or scalar publication emission order. |
| Existing edge publication, edge-copy source, and current-block join source queries. | Oracle for Route 5 edge and join-source identity equivalence. | BIR owns CFG/value/source identity; prepared move execution data remains the scheduling oracle, not schema. |
| Existing call argument/result source materialization, direct-global dependency, and publication-source routing reads. | Oracle for Route 6 call-use semantic source equivalence. | BIR owns call-use source facts only where ABI/layout is not required. ABI-bound publication-routing reads stay prepared/codegen-owned. |
| Existing fused-compare operand and materialized-condition producer queries. | Oracle for Route 7 comparison/condition provenance equivalence. | BIR owns operand/condition producer provenance, not fused-compare legality, condition-code selection, branch spelling, or final record errors. |

### Target-Facing Data That Must Stay Outside BIR

| Data family | Must remain outside durable BIR schema because |
| --- | --- |
| Physical register names, register banks/views, occupied register sets, emitted-register state, and register availability. | They are allocation/codegen state and differ by target. |
| Frame slot ids, concrete stack offsets, stack range, stack size/align, dynamic-stack plans, and outgoing stack area sizing. | They are frame/layout decisions, not target-neutral BIR relationships. |
| ABI placement for call arguments/results, variadic FPR count, preservation/clobber sets, byval aggregate lanes, aggregate transport lane layout, helper protocols, and carrier resources. | They encode ABI and target transport policy. |
| Target addressing legality, base-plus-offset usability, TLS register/relocation spelling, GOT/direct/page-low policy, global materialization policy, and AArch64 memory operand formation. | They are target addressing/lowering decisions. |
| Parallel-copy move bundle order, cycle temporary routing, execution site/block placement, phase/carrier policy, coalescing/redundancy flags, storage-sharing checks, and prepared move records. | They are copy-scheduling and storage policy. |
| Fused-compare legality, condition-code selection, branch emission strategy, hazard handling, final instruction records/errors, spill/reload pseudo selection, and final instruction order. | They are MIR/codegen emission behavior. |
| Runtime helper, special-carrier, variadic, dynamic-stack, atomic/f128 transport, scratch-resource, and prologue/epilogue planning facts. | They are target/helper resource plans even when the original operation is represented in BIR. |

### Blockers And Deferred Schema Questions

| Route or family | Blocker | Required follow-up shape |
| --- | --- | --- |
| Producer/source identity foundation | The current producer kind vocabulary is publication-owned in prepared code. | Define a target-neutral BIR producer-kind vocabulary before any schema prototype imports producer kinds. |
| Function-level lookup placement | Several accepted facts need cheap lookup by function, block, value, before-instruction index, call-site position, or edge key. | Add indexes only as references to typed annotation payloads; do not create a function-level lowering-plan container. |
| Block-entry/current-block publication | Existing prepared data is mixed with register-view and publication hook mechanics. | Prototype a block/value availability relationship that owns only source/value/producer identity. |
| CFG edge publication and join-source identity | Edge semantic facts are entangled with parallel-copy scheduling and move records. | Prototype edge/block relationship annotations before switching edge-copy source discovery; keep move scheduling prepared/MIR-owned. |
| Call-boundary source facts | Some publication-routing source-selection reads remain ABI/layout-bound. | Split semantic call-use source facts from ABI-bound reads; switch only materialization/routing consumers that no longer need placement data. |
| Memory/access identity | Existing address records combine BIR memory identity with frame layout, relocation, and target addressing policy. | Use BIR slot/name/global/pointer/string identity for semantic access; leave layout/addressing materialization to prealloc/codegen. |
| Module-level metadata | No accepted Phase A route requires durable module-level lowering metadata. | Keep module-level schema rejected unless a later phase identifies a genuinely module-scoped, target-neutral relationship. |
| Return-chain facts | Phase A did not establish return-chain as one of the seven accepted semantic routes. | Keep return-chain helper data private/cache-only until a separate source idea proves a target-neutral return relationship. |

## Route-Level Non-Schema Summary

| Route | Accepted schema candidate | Reject/cache/oracle boundary |
| --- | --- | --- |
| 1. Producer/source identity foundation | Value/instruction producer and materialization identity plus function-local lookup index. | Reject homes, storage, register availability, operand views, spill/reload behavior, final instruction order, and publication-owned enum authority. Prepared same-block and constant queries remain migration oracles. |
| 2. Select-chain and direct-global dependency identity | Value/instruction select-root, root instruction, direct `LoadGlobalInst`, and scalar eligibility facts. | Reject target materialization cost, hazard decisions, register availability, publication routing policy, call ABI behavior, and final move/branch choices. Prepared select-chain queries remain oracles. |
| 3. Memory/access semantic identity | Instruction/value memory access and source identity. | Reject frame slot ids, offsets, size/align, base-plus-offset legality, TLS/relocation spelling, target addressing-mode choice, and AArch64 operand legality. Prepared memory/access queries remain oracles. |
| 4. Block-entry and current-block publication identity | Block/value availability and source/producer identity. | Reject hooks, destination homes, storage encoding, stack-source extension, register-view conversion, immediate publication payloads, emitted storage availability, and scalar publication emission policy. Current prepared publication reads remain oracles. |
| 5. CFG edge publication and join-source identity | Edge/block predecessor-successor-destination-source identity. | Reject move order, cycle temporaries, execution site, phase/carrier policy, coalescing/redundancy, destination registers, storage-sharing checks, and prepared moves. Prepared edge/join queries remain oracles. |
| 6. Call-boundary semantic source facts | `CallInst` argument/result source facts plus value/index references. | Reject ABI register/stack placement, outgoing stack sizing, variadic FPR count, preservation/clobber sets, byval lanes, scratch requirements, destination homes, helper/carrier protocols, and aggregate transport layout. Prepared call plans remain oracles for ABI-bound behavior. |
| 7. Comparison and materialized-condition producer identity | Instruction/terminator comparison and branch-condition provenance. | Reject fused-compare legality, condition-code selection, branch strategy, final instruction records/errors, hazard handling, emitted-register state, and AArch64 compare/branch instruction selection. Prepared comparison queries remain oracles. |

## Follow-Up Schema Ideas

Step 4 turns only Phase A-proven relationships and Step 3 blockers into later
schema work. Each linked idea is a prototype contract, not an implementation
change in this analysis phase.

| Domain | Follow-up idea | Phase A / Phase B justification | Boundary to preserve |
| --- | --- | --- | --- |
| Producer/source identity foundation | `ideas/open/159_bir_producer_identity_annotation_schema.md` | Route 1 proved same-block producer, constant, value/name/type, producer index, materialization, and producer-kind relationships. Step 3 recorded that producer-kind authority is still a schema blocker. | Do not import homes, registers, storage state, emitted-register availability, spill/reload behavior, frame slots, or final instruction order. |
| Select-chain and direct-global dependency identity | `ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md` | Route 2 proved select-root, root instruction/index, direct `LoadGlobalInst`, scalar eligibility, and combined lookup relationships. | Do not import target materialization cost, hazards, register availability, publication routing, call ABI behavior, or final move/branch choices. |
| Memory/access semantic identity | `ideas/open/161_bir_memory_access_identity_annotation_schema.md` | Route 3 proved direct memory access, same-block global-load, load-local source, result/stored value, address-space/volatile, semantic base, and source identity relationships. | Do not import `PreparedAddress` wholesale, frame slots, offsets, size/align, relocation spelling, target addressing legality, or AArch64 memory operand policy. |
| Block-entry and current-block publication identity | `ideas/open/162_bir_publication_availability_annotation_schema.md` | Route 4 proved block/current value availability, source producer, produced value/name, producer index, and source-producer kind relationships. | Do not import hooks, destination homes, storage encoding, stack-source extension policy, register-view conversion, immediate payload spelling, emitted storage state, or emission order. |
| CFG edge publication and join-source identity | `ideas/open/163_bir_edge_join_source_annotation_schema.md` | Route 5 proved predecessor/successor/destination/source identity, source producer block/instruction, optional memory source, no-source marker, and current-block join-source relationships. | Do not import move bundle order, cycle temporary routing, execution-site policy, carrier/phase policy, coalescing/redundancy flags, destination registers, storage sharing, or prepared move records. |
| Call-boundary semantic source facts | `ideas/open/164_bir_call_use_source_annotation_schema.md` | Route 6 proved call argument/result semantic source facts for source-producer and direct-global dependency families. Step 3 recorded ABI/layout-bound reads as non-schema. | Do not import ABI register/stack placement, outgoing-stack sizing, variadic FPR count, clobbers, byval lanes, scratch, destination homes, helper/carrier protocols, or aggregate transport layout. |
| Comparison and materialized-condition producer identity | `ideas/open/165_bir_comparison_condition_annotation_schema.md` | Route 7 proved comparison producer, branch condition, lhs/rhs producer or integer constant, `BinaryInst`, condition value, instruction index, fused-operand, and materialized-condition provenance. | Do not import fused-compare legality, condition-code selection, branch strategy, final instruction records/errors, hazard handling, emitted-register state, or target compare/branch spelling. |
| Function-local annotation lookup indexes | `ideas/open/166_bir_annotation_lookup_index_schema.md` | Step 2 and Step 3 both found that accepted relationships need cheap lookup by function, block, value, instruction index, call-site position, edge key, and relationship kind. | Indexes may point at typed annotation payloads, but must not duplicate payloads or become a BIR-owned `PreparedFunctionLookups` lowering-plan container. |
| Module-level lowering metadata | No follow-up idea justified by Phase A. | Step 2 found no accepted Phase A route fact that requires durable module-level metadata. | Keep module-level schema rejected unless a later phase proves a genuinely module-scoped, target-neutral relationship. |
| Return-chain helper facts | No follow-up idea justified by Phase A. | Step 3 found no standalone return-chain semantic route among the seven accepted Phase A groups. | Keep return-chain helper data private/cache-only until a separate source idea proves a target-neutral return relationship. |

## AArch64 Stability Migration Constraints

Introducing BIR-owned annotations must not change AArch64 output merely because
the query source moved from `Prepared*` records to BIR. Future schema prototype
work should use these constraints as closure gates:

| Constraint | Required migration behavior |
| --- | --- |
| Keep prepared queries as oracles until each replacement proves equivalence. | For every switched consumer, compare the BIR annotation answer against the existing prepared same-block, select-chain, memory, publication, edge, call, or comparison query over representative positive and negative cases before deleting the prepared path. |
| Preserve cheap lookup identity. | Typed annotations may be indexed at function scope, but consumers must retain the same key shapes they rely on today: function, block label, value/name, before-instruction index, call-site ordinal, edge key, and relationship kind. Migration must not replace indexed prepared reads with broad scans. |
| Move only target-neutral identity first. | Switch producer, source, value/name/type, block/edge, instruction-index, address-space/volatile, semantic base/source, direct-global, and comparison provenance facts before any AArch64 lowering policy. |
| Keep target and ABI policy downstream. | Physical registers, homes, frame slots, stack offsets, ABI argument/result placement, outgoing stack size, variadic counts, helper/carrier protocols, branch spelling, memory operand legality, and move scheduling must continue to come from prealloc/MIR/codegen structures. |
| Preserve emission order and record-error behavior. | Annotation introduction must not reorder final instructions, alter fused-compare legality, change condition-code selection, create or suppress spill/reload pseudos, or rewrite record-error decisions. |
| Bridge one relationship family at a time. | Each follow-up idea should switch a bounded query family with prepared fallback still available, then remove the fallback only after AArch64 behavior and negative-case answers match. |
| Keep cross-target schema shape neutral. | Schema fields should be expressible without AArch64 register names, relocation spelling, instruction mnemonics, scratch policy, or addressing-mode names so later x86/riscv consumers can reuse the same payload. |

## Closure Readiness Checklist

| Required output | Artifact location | Status |
| --- | --- | --- |
| Link to this Phase B artifact. | This document: `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`. | Present. |
| Phase A route coverage table for all seven routes. | `## Phase A Route Coverage Table` and `## Placement Summary By Phase A Route`. | Present; every route has evidence, closed-route source, accepted relationship, prepared surface, boundary, and classification status. |
| Candidate BIR annotation schema map. | `## Candidate BIR Annotation Schema Map`. | Present across value, instruction, terminator, block, edge, function, and module-level placement decisions. |
| Private/cache-only `Prepared*` classifications. | `### Whole Prepared Struct Copy Rejects` and `### Private Or Cache-Only Data`. | Present; whole prepared structs are rejected and caches are separated from durable payloads. |
| Bridge/oracle data. | `### Bridge And Oracle Data`. | Present; old prepared query families remain comparison/fallback surfaces during migration. |
| Target-facing fields that must not enter BIR. | `### Target-Facing Data That Must Stay Outside BIR`. | Present; register, frame, ABI, addressing, move, branch, helper, and final-instruction policy is excluded. |
| Blockers and deferred schema questions. | `### Blockers And Deferred Schema Questions`. | Present; blocker domains are tied to required follow-up shape or explicit no-follow-up decisions. |
| Follow-up schema prototype ideas. | `## Follow-Up Schema Ideas`. | Present; eight justified follow-ups are linked and module-level/return-chain non-follow-ups are recorded. |
| Migration constraints for AArch64 stability. | `## AArch64 Stability Migration Constraints`. | Present; future prototypes must preserve prepared-oracle equivalence, lookup cost, target neutrality, and AArch64 emission behavior. |
| Traceability back to Phase A. | `## Provenance`, `## Evidence Source List`, per-route table rows, schema-map traceability cells, and follow-up idea justifications. | Present; every accepted schema candidate points to a Phase A route and closure note. |

## Closure Summary

Phase B classifies the seven Phase A BIR-owned semantic routes into durable BIR
annotation candidates, non-schema prepared/cache data, bridge/oracle data,
target-facing rejects, and follow-up schema prototype domains. The accepted
schema surface is intentionally narrow: value, instruction, terminator, block,
edge, and function-local index annotations may own target-neutral relationship
facts, while module-level lowering metadata remains rejected for this evidence
set.

The audit rejects copying whole `Prepared*` structures into BIR. Future work
should prototype the eight linked schema ideas, preserving the prepared query
families as AArch64 equivalence oracles until each relationship-specific switch
proves identical answers and preserves cheap lookup identity. No implementation
or schema code changes are required to close this Phase B analysis plan.
