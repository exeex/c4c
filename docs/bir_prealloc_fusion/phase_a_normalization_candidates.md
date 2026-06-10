# Phase A BIR Normalization Candidates

Source idea: `ideas/open/151_phase_a_bir_normalization_candidate_audit.md`
Status: Step 1 evidence inventory initialized.

This artifact is the durable Phase A handoff for classifying which
`Prepared*` facts are target-neutral BIR normalization relationships and which
facts must stay in prealloc, MIR, ABI, stack layout, or target codegen.

## Evidence Inventory

### Inspected Inputs

| Domain | Files or notes inspected | Evidence captured |
| --- | --- | --- |
| Phase contract | `ideas/open/151_phase_a_bir_normalization_candidate_audit.md`, `plan.md`, `docs/bir_prealloc_fusion/README.md` | Phase A is analysis-only and must produce candidate, reject, dependency, follow-up, and proof-route handoff data in this file. |
| Prepared aggregate surface | `src/backend/prealloc/module.hpp`, `src/backend/prealloc/prepared_lookups.hpp` | `PreparedBirModule` aggregates the raw `bir::Module`, target profile, names, control flow, value locations, stack layout, addressing, liveness, regalloc, frame/dynamic stack/call/store-source/variadic/storage/special-carrier/runtime-helper plans, completed phases, and notes. `PreparedFunctionLookups` remains the one-pass aggregate lookup bundle for call plans, address materializations, memory accesses, move bundles, return chains, value homes, edge publications, and source producers. |
| Same-block and select-chain queries | `src/backend/prealloc/select_chain_lookups.hpp`, `src/backend/prealloc/calls.hpp` | Semantic query structs/APIs include `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`, `PreparedCurrentBlockPublicationConsumption`, `PreparedSelectChainDependencyQuery`, `PreparedScalarSelectChainMaterialization`, `PreparedDirectGlobalSelectChainDependency`, and helpers such as `find_prepared_same_block_scalar_producer`, `evaluate_prepared_same_block_integer_constant`, `find_prepared_direct_global_select_chain_dependency`, `find_prepared_select_chain_source_producer`, and `find_prepared_scalar_select_chain_materialization`. |
| Edge publication and current-block source facts | `src/backend/prealloc/publication_plans.hpp` | Semantic-ish fact surfaces include `PreparedEdgePublicationSourceProducer`, `PreparedEdgePublicationSourceProducerLookups`, `PreparedEdgePublication`, `PreparedEdgePublicationLookups`, `PreparedEdgeCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceFact(s)`, and query helpers such as `make_prepared_edge_publication_lookups`, `make_prepared_edge_publication_source_producer_lookups`, `find_indexed_prepared_edge_publication_source_producer`, `prepare_edge_copy_source_facts`, `prepare_block_entry_parallel_copy_edge_source_facts`, and `prepare_current_block_join_parallel_copy_source_facts`. |
| Publication plans with target/layout payloads | `src/backend/prealloc/publication_plans.hpp`, `src/backend/prealloc/formal_publications.hpp` | Mixed surfaces include `PreparedScalarPublicationPlan`, `PreparedStoreSourcePublicationPlan`, `PreparedTypedStackSourcePublication`, `PreparedAggregateStackSourceAuthority`, and `PreparedFormalPublicationPlan`. They combine source/destination identity with home kind, frame slot, stack offset/size/align, register placement/bank, and current-block publication references. |
| Call-boundary facts | `src/backend/prealloc/calls.hpp`, `src/backend/prealloc/call_plans.cpp` | Candidate semantic fields include call-argument source selection, direct-global select-chain dependency, source-producer materialization, and publication source routing. Rejected target/layout payloads include destination register names, occupied register sets, stack offsets/sizes, ABI placements, scratch requirements, and aggregate transport lane layout. |
| Memory/addressing facts | `src/backend/prealloc/addressing.hpp`, `src/backend/prealloc/stack_layout/stack_layout.hpp` | Semantic access surfaces include `PreparedAddress`, `PreparedMemoryAccess`, `PreparedSameBlockGlobalLoadAccess`, `PreparedSameBlockLoadLocalStoredValueSource`, and lookup helpers for same-block load/store source identity. Target/layout payloads include concrete frame slot ids, stack offsets, TLS relocation/register spelling, global materialization policy, and base-plus-offset usability. |
| Comparison facts | `src/backend/prealloc/comparison.hpp` | `PreparedFusedCompareOperandProducer`, `PreparedFusedCompareOperandProducerFacts`, and `PreparedMaterializedConditionProducer` already name comparison-specific producer relationships over same-block/source-producer facts and constants. |
| BIR data surface | `src/backend/bir/bir.hpp`, `src/backend/bir/lir_to_bir/*.cpp`, `src/backend/bir/lir_to_bir/memory/*.cpp` | BIR owns `Module`, `Function`, `Block`, `Inst`, `Terminator`, typed values, `PhiIncoming`, `CallInst`, `LoadLocalInst`, `LoadGlobalInst`, `StoreLocalInst`, `StoreGlobalInst`, `MemoryAddress`, ids for names/slots/blocks/links, intrinsic/atomic metadata, and compare/select/call/address payloads. Current BIR has nodes/blocks/functions but no durable prepared-style annotation surface for producer/publication/edge relationship facts. |
| AArch64 prepared consumers | `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`, `dispatch_producers.cpp`, `dispatch_value_materialization.cpp`, `dispatch_edge_copies.cpp`, `calls.cpp`, `comparison.cpp`, `memory.cpp`, `memory_store_retargeting.cpp`, `operands.cpp`, `instruction.cpp`, `prologue.cpp`, `atomics.cpp`, `f128.cpp` | AArch64 consumes prepared facts for publication, same-block materialization, select-chain dependencies, call materialization/routing, memory retargeting, decoded homes, frame/prologue layout, atomic/f128 carriers, and final instruction records. Emission policy, register view/spelling, scratch selection, ABI lowering, and final instruction order remain target-local consumers, not BIR candidates. |
| Closure notes 130-150 | `ideas/closed/130_*.md` through `ideas/closed/150_*.md` | Recent work moved AArch64 rediscovery into shared prepared query surfaces, then narrowed owners: select-chain in `select_chain_lookups.hpp`, edge publication in `publication_plans.hpp`, call argument materialization in `calls.hpp`, comparison in `comparison.hpp`, addressing load-local source in `addressing.hpp`, stack id helpers in stack-layout ownership, and residual `prepared_lookups.hpp` pressure down to true aggregate/facade use. |

### Prepared Facts Inventory

| Fact group | Current owner | Representative structs/fields/helpers | Initial ownership read |
| --- | --- | --- | --- |
| Same-block scalar producer/materialization | `select_chain_lookups.hpp/.cpp` | `PreparedSameBlockScalarProducer::{producer,instruction,instruction_index,value_name}`, `PreparedSameBlockValueMaterializationQuery`, `find_prepared_same_block_scalar_producer`, `evaluate_prepared_same_block_integer_constant` | Strong BIR-normalization candidate. It describes producer/value identity inside an ordered BIR block before MIR emission. |
| Select-chain dependency | `select_chain_lookups.hpp/.cpp`, with type from `calls.hpp` | `PreparedDirectGlobalSelectChainDependency::{contains_direct_global_load,root_is_select,root_instruction_index}`, `PreparedScalarSelectChainMaterialization`, `find_prepared_direct_global_select_chain_dependency`, `find_prepared_scalar_select_chain_materialization` | Strong BIR-normalization candidate. It relates BIR select/global-load producer structure, not target registers. |
| Current-block publication consumption | `select_chain_lookups.hpp/.cpp`, `value_locations.hpp` | `PreparedCurrentBlockPublicationConsumption::{available,source_producer,instruction,produced_value,instruction_index,value_name,source_producer_kind}` | Candidate with dependency on a BIR publication/entry relationship schema. Keep register-view emission outside BIR. |
| Edge publication and edge-copy source facts | `publication_plans.hpp/.cpp` | `PreparedEdgePublication`, `PreparedEdgePublicationLookups`, `PreparedEdgeCopySourceFacts::{predecessor_label,successor_label,destination_value,source_value,source_producer_*,source_memory_*}`, `prepare_edge_copy_source_facts` | Candidate for edge relationship ownership when limited to predecessor/successor/destination/source/producer identity. Reject move-routing fields that encode parallel-copy execution and homes. |
| Current-block join parallel-copy source facts | `publication_plans.hpp/.cpp` | `PreparedCurrentBlockJoinParallelCopySourceFact(s)`, `prepare_current_block_join_parallel_copy_source_facts` | Mixed. Source/incoming expression identity can become BIR relationship data; AArch64 routing booleans and register-sharing checks should stay target/prealloc-local. |
| Scalar/store publication plans | `publication_plans.hpp/.cpp` | `PreparedScalarPublicationPlan`, `PreparedStoreSourcePublicationPlan`, `plan_prepared_scalar_publication`, `plan_prepared_store_source_publication`, `plan_pending_prepared_store_global_publications` | Mixed. Source/destination value and semantic memory access identity are candidates; concrete homes, stack offsets, frame slots, and publication hook/storage encodings are rejects. |
| Call-boundary semantic facts | `calls.hpp/.cpp` | `PreparedCallArgumentSourceSelection`, `PreparedCallArgumentSourceProducerMaterialization`, `PreparedCallArgumentDirectGlobalSelectChainDependency`, `PreparedCallArgumentPublicationSourceRouting` | Candidate only for source identity, dependency, and materializable-producer relationships. ABI binding, register names, stack area layout, aggregate lane transport, and scratch are rejects. |
| Memory/access identity facts | `addressing.hpp/.cpp`, stack-layout helpers | `PreparedAddress`, `PreparedMemoryAccess`, `PreparedSameBlockGlobalLoadAccess`, `PreparedSameBlockLoadLocalStoredValueSource`, `find_prepared_same_block_load_local_source_producer` | Candidate for semantic access identity, source/stored-value relation, address-space/volatile facts, and local/global/pointer source relationships. Reject concrete frame layout, relocation, target addressing-mode choice, and base-plus-offset legality. |
| Comparison/materialized-condition facts | `comparison.hpp/.cpp` | `PreparedFusedCompareOperandProducer`, `PreparedFusedCompareOperandProducerFacts`, `PreparedMaterializedConditionProducer`, `find_prepared_fused_compare_operand_producer_facts`, `find_prepared_materialized_condition_producer` | Strong BIR-normalization candidate because it names operand/condition producer relationships over BIR compares and same-block producers. |
| Value homes, storage, regalloc, frame, dynamic stack, special carriers | `value_locations.hpp`, `storage.hpp`, `regalloc.hpp`, `frame.hpp`, `dynamic_stack.hpp`, `special_carriers.hpp`, `runtime_helpers.hpp`, `variadic.hpp` | `PreparedValueHome`, `PreparedStoragePlanValue`, `PreparedRegisterPlacement`, `PreparedFrameSlot`, `Prepared*Carrier`, `Prepared*RuntimeHelper` | Mostly rejected from BIR normalization. These encode allocation/layout/ABI/helper emission policy, though some semantic carrier source identity may depend on future annotation phases. |

### BIR Surface Inventory

| Surface | Existing data | Normalization implication |
| --- | --- | --- |
| Node/instruction data | `BinaryInst`, `SelectInst`, `CastInst`, `PhiInst`, `CallInst`, `LoadLocalInst`, `LoadGlobalInst`, `StoreLocalInst`, `StoreGlobalInst` | Enough raw node data exists to compute many producer, select, compare, call, and memory relationships before prealloc. Missing piece is durable relationship storage/query authority. |
| Block/function data | `Block::{label,insts,terminator,label_id}`, `Function::{params,local_slots,blocks,atomic_operations}`, `Terminator` label ids and values | Edge, predecessor/successor, entry-publication, and current-block relationships can be owned next to block/function structure if a schema is added later. |
| Value/name identity | `Value`, `NameTables`, `ValueNameId`, `BlockLabelId`, `SlotNameId`, `LinkNameId` | Semantic identity can be expressed without target registers or frame offsets. |
| Memory/call metadata | `MemoryAddress`, `CallArgAbiInfo`, `CallResultAbiInfo`, `IntrinsicOperation`, `AtomicOperation` | Some call/memory semantic facts already exist in BIR. Target ABI placement and final materialization remain outside BIR. |

### Closure Notes 130-150 Inventory

| Closed idea | Relevant closure signal |
| --- | --- |
| 130 | Dispatch audit found shared-contract gaps around producer, publication, edge-copy, current-block, and select-chain facts; target-local emission should remain AArch64-owned. |
| 131 | Privatized AArch64 edge-copy helper surface while preserving prepared edge-copy/publication facts. |
| 132 | Trimmed AArch64 dispatch lookup surface; retained `make_named_prepared_result_register` and `emitted_scalar_value_available` as real hooks. |
| 133 | Extracted shared prepared value-home query and identified remaining prepared ids, return chains, move bundles, call plans, memory accesses, preserved values, edge producers, select-chain, memory-access, and call-plan opportunities. |
| 134 | Shared same-block materialization and select-chain dependency query facades; kept emission/hazard behavior target-local. |
| 135 | Added shared current-block entry publication query while preserving AArch64 register-view and operand spelling. |
| 136 | Analysis-only audit classified prepared-query surfaces and produced owner-cleanup follow-ups. |
| 137 | Made `select_chain_lookups.hpp` the public owner for select-chain query declarations and types. |
| 138 | Moved call-plan lookup indexes through call-domain ownership while preserving call semantics. |
| 139 | Moved addressing lookup ownership toward addressing/memory ownership. |
| 140 | Split reusable edge publication/source/home/move facts from AArch64-shaped current-block routing conveniences. |
| 141 | Residual audit kept `PreparedFunctionLookups` and return-chain helpers under `prepared_lookups.*`, and created follow-ups 142-149. |
| 142 | Moved value-home, move-bundle, ABI/result-lane/current-block-entry-publication lookup declarations toward value-location ownership. |
| 143 | Moved stack-layout id helpers to stack-layout ownership. |
| 144 | Moved source-producer and same-block materialization APIs to the narrow select-chain/source-producer owner. |
| 145 | Split reusable current-block join source facts from AArch64 instruction-routing convenience. |
| 146 | Moved call-argument source-producer materialization to call ownership, reusing same-block/source-producer facts. |
| 147 | Established `prealloc/comparison.hpp/.cpp` as the owner for fused-compare and materialized-condition producer facts. |
| 148 | Moved same-block load-local stored-value lookup declaration to addressing ownership while reusing source-producer, stack-layout, and memory-access facts. |
| 149 | Cleaned residual broad `prepared_lookups.hpp` includes and left only the edge-publication lookup declaration owner follow-up. |
| 150 | Moved `make_prepared_edge_publication_lookups` declaration to `publication_plans.hpp`, leaving remaining prepared lookup includes justified by true facade/aggregate ownership. |

## BIR-Normalization Candidates

Step 2 will fill this table. Initial candidate groups from Step 1 inventory:

| Candidate group | Current owner | Current consumers | Proposed BIR owner surface | Why semantic |
| --- | --- | --- | --- | --- |
| Same-block producer/materialization | TBD | TBD | TBD | TBD |
| Select-chain dependency/materialization | TBD | TBD | TBD | TBD |
| Current-block entry publication consumption | TBD | TBD | TBD | TBD |
| Edge publication source identity | TBD | TBD | TBD | TBD |
| Call-boundary source/dependency identity | TBD | TBD | TBD | TBD |
| Memory/access source identity | TBD | TBD | TBD | TBD |
| Comparison/materialized-condition producer identity | TBD | TBD | TBD | TBD |

## Facts Rejected From BIR Normalization

Step 3 will fill this table. Initial reject groups from Step 1 inventory:

| Fact group | Current owner | Reject reason | Required non-BIR owner |
| --- | --- | --- | --- |
| ABI register/stack placement | TBD | Target ABI placement and concrete calling convention lowering. | Prealloc/call plans, ABI, AArch64 codegen |
| Stack offsets, frame slots, saved-register placement | TBD | Concrete frame layout and prologue/epilogue policy. | Stack layout, frame plan, AArch64 prologue |
| Register spelling, banks, views, occupied registers | TBD | Target allocation and operand spelling. | Regalloc, target ABI, AArch64 operands/instruction |
| Scratch policy and aggregate transport lanes | TBD | Target emission and temporary resource policy. | Prealloc call plans and AArch64 codegen |
| Target addressing modes, TLS relocations, base-plus-offset legality | TBD | Target relocation/addressing decision. | Addressing/prealloc plus target codegen |
| Final instruction routing/order/hazard policy | TBD | MIR emission policy. | AArch64 dispatch/codegen |

## Dependency Order

Step 4 will replace this scaffold with accepted route order.

1. Establish BIR-owned producer/source identity relationships before consumer
   switch work.
2. Add BIR-owned publication/edge relationship surfaces only after producer
   identity has a stable schema.
3. Add call, memory, and comparison relationships on top of producer and
   publication identity.
4. Switch MIR consumers only after each equivalent BIR-owned query exists and
   rejects target-local policy migration.

## Follow-Up Idea Payloads

Step 5 will draft concrete follow-up idea payloads here.

| Proposed idea filename or prefix | Route | Scope | Out of scope | Acceptance criteria | Proof route | Reviewer reject signals |
| --- | --- | --- | --- | --- | --- | --- |
| TBD | Same-block producer/materialization | TBD | TBD | TBD | TBD | TBD |
| TBD | Select-chain dependency/materialization | TBD | TBD | TBD | TBD | TBD |
| TBD | Current-block/edge publication identity | TBD | TBD | TBD | TBD | TBD |
| TBD | Call-boundary semantic source facts | TBD | TBD | TBD | TBD | TBD |
| TBD | Memory/access semantic identity | TBD | TBD | TBD | TBD | TBD |
| TBD | Comparison/materialized-condition producer facts | TBD | TBD | TBD | TBD | TBD |

## Proof-Route Recommendations

Step 5 will complete this section per follow-up idea.

| Route | Suggested proof | Escalation trigger |
| --- | --- | --- |
| Same-block/select-chain relationships | Matching `test_before.log`/`test_after.log` around affected backend subset plus targeted AArch64 consumers. | Any semantic query behavior change or nearby same-feature support risk. |
| Publication/edge relationships | Matching backend subset covering dispatch publication and edge copies. | Changes to parallel-copy authority, publication ordering, or control-flow transfer. |
| Call-boundary relationships | Matching backend call/codegen subset. | ABI placement, outgoing stack, aggregate transport, or runtime helper behavior changes. |
| Memory/access relationships | Matching backend memory/codegen subset. | Addressing mode, frame offset, TLS/global relocation, or store-retargeting behavior changes. |
| Comparison relationships | Matching backend comparison/branch subset. | Branch condition, fused compare, or materialized condition semantics change. |
