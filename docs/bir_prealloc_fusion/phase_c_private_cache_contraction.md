# Phase C Private Cache Contraction

Source idea: `ideas/open/153_phase_c_prealloc_private_cache_contraction_audit.md`
Status: Step 5 final closure summary complete.

This artifact is the durable Phase C audit surface for deciding which
remaining prealloc lookup, index, plan, and query surfaces can become private
caches or thin pass internals now that BIR owns the Phase A/B semantic
relationships.

Phase C is analysis-only. This file records prerequisites and contraction
readiness; it does not delete prepared APIs, switch consumers, or change
implementation source.

## Prerequisite Source Map

| Source | Phase C role |
| --- | --- |
| `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` | Defines the seven target-neutral semantic routes, dependency order, reject boundaries, migration-oracle rule, and proof-route expectations. |
| `ideas/closed/151_phase_a_bir_normalization_candidate_audit.md` | Confirms Phase A closure and the durable handoff sections for candidate groups, rejects, dependency order, follow-up payloads, and proof routes. |
| `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md` | Classifies each Phase A route into BIR annotation candidates, cache/private data, bridge/oracle data, target-facing rejects, blockers, and schema follow-ups. |
| `ideas/closed/152_phase_b_bir_annotation_schema_candidate_audit.md` | Confirms Phase B analysis closure and that no implementation changed during Phase B analysis. |
| `ideas/closed/159_bir_producer_identity_annotation_schema.md` | Route 1 implementation closure: producer identity records, typed BIR vocabulary, `Route1ProducerIndex`, and shared MIR producer/constant query migration. |
| `ideas/closed/160_bir_select_chain_global_dependency_annotation_schema.md` | Route 2 implementation closure: select-chain/direct-global records, lookup/index helpers, and shared MIR select-chain query migration. |
| `ideas/closed/161_bir_memory_access_identity_annotation_schema.md` | Route 3 implementation closure: memory/access records, lookup/index helpers, and shared MIR memory/access query consumers. |
| `ideas/closed/162_bir_publication_availability_annotation_schema.md` | Route 4 implementation closure: publication availability records/indexes, current-block query migration, and block-entry residual consumer. |
| `ideas/closed/163_bir_edge_join_source_annotation_schema.md` | Route 5 implementation closure: edge/join records/indexes, edge publication source query migration, and join-source residual consumer. |
| `ideas/closed/164_bir_call_use_source_annotation_schema.md` | Route 6 implementation closure: call-use records/indexes, source-producer helper migration, and no broad MIR/codegen/prealloc consumer switch. |
| `ideas/closed/165_bir_comparison_condition_annotation_schema.md` | Route 7 implementation closure: comparison/branch-condition records/indexes, materialized-condition helper migration, and branch/codegen residual boundaries. |
| `ideas/closed/166_bir_annotation_lookup_index_schema.md` | Route index facade closure: reference validation and private facade coverage for selected Route 4 and Route 7 references only. |

## Route Prerequisite Map

| Route | Phase A/B prerequisite | Implemented BIR annotation and index surface | Residual consumers and cache pressure | Oracle coverage | Explicit blockers for contraction |
| --- | --- | --- | --- | --- | --- |
| 1. Producer/source identity foundation | Phase A accepted same-block producer nodes, immediate constants, source value/name/type identity, producer instruction index, materialization availability, and target-neutral producer-kind vocabulary. Phase B classified these as value/instruction annotations with a function index. | `Route1ProducerKind`, `Route1SourceValueIdentity`, `Route1ImmediateIntegerConstant`, `Route1ProducerInstructionIdentity`, `Route1MaterializationAvailability`, `Route1ProducerRecord`, `Route1ProducerIndex`, `Route1SameBlockProducerQuery`, `route1_build_producer_index`, `route1_find_same_block_scalar_producer`, `route1_find_materialization_availability`, and `route1_evaluate_same_block_integer_constant`. | Generic MIR same-block producer and integer-constant identity reads moved to the BIR Route 1 query surface. Prepared producer and constant helpers still need to remain as migration oracles and compatibility facades while higher routes and AArch64 consumers finish moving. | Closure notes cite oracle-equivalence coverage in `backend_prepared_lookup_helper`, plus narrow producer proof through `backend_prepared_lookup_helper` and `backend_x86_shared_producer_query`; broad backend regression stayed at `179/179`. | Do not contract away prepared producer or constant query surfaces until downstream route consumers no longer need them as comparison oracles. Do not privatize by importing homes, registers, storage, emitted-register state, operand views, frame slots, or final instruction order into BIR. |
| 2. Select-chain and direct-global dependency identity | Phase A accepted select-root, direct `LoadGlobalInst` dependency, root producer, root instruction index, scalar materialization eligibility, and combined identity lookup. Phase B classified these as value/instruction annotations with function-level indexes. | `Route2SelectChainProducerKind`, `Route2SelectChainProducerRecord`, `Route2SelectChainDirectGlobalDependencyRecord`, `Route2SelectChainValueRecord`, `Route2SelectChainValueIndex`, `Route2SelectChainValueQuery`, `route2_build_select_chain_value_index`, and `route2_find_select_chain_value_record`. | Shared MIR select-chain query consumer moved to the BIR-backed index. Prepared select-chain helpers still carry oracle and compatibility pressure for call/publication/materialization users that have not been audited in Phase C. | Closure notes state prepared select-chain answers remain oracle tests for accepted positive and negative cases; broad backend validation stayed at `179/179` with regression guard `PASS`. | Do not contract prepared select-chain surfaces while any consumer still needs prepared answers to compare BIR direct-global, non-select-root, no-dependency, or scalar-eligibility cases. Do not move materialization cost, hazard decisions, register availability, publication routing, call ABI behavior, or final move/branch choices into the BIR surface. |
| 3. Memory/access semantic identity | Phase A accepted direct memory access, same-block global-load access, load-local stored-value source, result/stored value identity, address-space/volatile bits, semantic base kind, and local/global/pointer/string source identity. Phase B classified these as instruction/value annotations and rejected target layout/addressing. | `Route3MemoryAccessNodeKind`, `Route3MemoryAccessBaseKind`, `Route3MemoryAccessValueRole`, `Route3MemoryAccessRecord`, `Route3MemoryAccessValueRecord`, `Route3MemoryAccessIndex`, `Route3MemoryAccessQuery`, `Route3SameBlockGlobalLoadAccessRecord`, `Route3SameBlockLoadLocalSourceRecord`, `route3_build_memory_access_index`, `route3_find_memory_access_record`, `route3_find_same_block_global_load_access`, and `route3_find_same_block_load_local_source`. | Shared MIR memory/access query consumers are BIR-backed. Prealloc/addressing surfaces that mix semantic access identity with frame slots, offsets, relocation, TLS, base-plus-offset legality, and operand formation remain public or cache-backed until Step 2 inventories exact consumers. | Route review found no overfit or target-policy leakage; broad backend logs stayed at `179/179`; `git diff --check` passed. Prepared memory/access answers remain the migration oracle. | Contraction must not copy `PreparedAddress`/`PreparedMemoryAccess` wholesale into BIR, delete target-addressing data, or force consumers to rebuild expensive same-block lookup scans manually. Target/layout-specific memory facts remain out of scope. |
| 4. Block-entry and current-block publication identity | Phase A accepted current-block and block-entry value availability, source producer identity, produced value/name, producer instruction/index, and source-producer kind. Phase B classified these as block/value annotations and rejected publication mechanics. | `Route4PublicationAvailabilityStatus`, `Route4PublicationScope`, `Route4PublicationSourceKind`, `Route4PublicationValueRole`, `Route4CurrentBlockPublicationRecord`, `Route4BlockEntryPublicationRecord`, `Route4PublicationValueRecord`, `Route4PublicationAvailabilityIndex`, `route4_build_publication_availability_index`, `route4_find_current_block_publication`, and `route4_find_block_entry_publication`. | Low-risk current-block MIR query migration is complete. The block-entry MIR consumer still uses its older semantic PHI scan even though Route 4 block-entry records/indexes are implemented and oracle-covered. Prepared current-block/block-entry publication reads remain public migration oracles. | Closure notes cite current-block and block-entry oracle tests; reviewer found no blocking issues; broad backend validation stayed at `179/179`. Route index facade later added selected Route 4 reference validation. | Do not privatize block-entry prepared publication helpers until the residual block-entry MIR consumer moves to BIR Route 4 or an equivalent facade. Do not move hook kind, destination home, storage encoding, stack-source extension, register-view conversion, immediate payload spelling, emitted storage availability, or scalar publication emission policy into BIR. |
| 5. CFG edge publication and join-source identity | Phase A accepted predecessor/successor/destination/source identity, source producer block/instruction, optional memory-source identity, no-source identity, and current-block join-source facts. Phase B classified these as edge/block annotations and rejected parallel-copy scheduling. | `Route5PublicationStatus`, `Route5PublicationScope`, `Route5PublicationSourceKind`, `Route5PublicationValueRole`, `Route5CfgEdgePublicationRecord`, `Route5CurrentBlockJoinSourceRecord`, `Route5PublicationValueRecord`, `Route5EdgeJoinSourceIndex`, `route5_build_edge_join_source_index`, `route5_find_cfg_edge_publication`, and `route5_find_current_block_join_source`. | Low-risk CFG edge publication source MIR query migration is complete. `mir::find_bir_current_block_join_source_identity(...)` remains on its prior semantic implementation while Route 5 join records/indexes are implemented and oracle-covered. Current-block join-source public helpers therefore cannot be contracted yet. | Closure notes cite edge and join-source oracle tests, full-suite baseline acceptance at `3427/3427`, and broad backend logs at `179/179`. | Do not privatize current-block join-source prepared helpers until the residual public helper and consumers move to Route 5 records/indexes. Do not mix move bundle order, cycle temporary routing, execution site, phase/carrier policy, coalescing, redundancy, destination registers, storage-sharing checks, or prepared move records into BIR. |
| 6. Call-boundary semantic source facts | Phase A accepted call argument/result semantic source identity for source-producer and direct-global dependency families, plus eligible memory/publication references where target-neutral. Phase B classified these as `CallInst` instruction/value annotations and rejected ABI/layout-bound reads. | `Route6CallUseStatus`, `Route6CallUseSourceKind`, `Route6CallUseValueRole`, `Route6CallArgumentSourceRecord`, `Route6CallArgumentSourceProducerRecord`, `Route6CallArgumentDirectGlobalDependencyRecord`, `Route6CallArgumentPublicationSourceRecord`, `Route6CallResultSourceRecord`, `Route6CallResultLaneSourceRecord`, and `Route6CallUseSourceIndex`. | `bir::find_call_argument_source_producer_materialization(...)` moved to Route 6 records. Closure explicitly notes there was no dedicated public MIR call-use query in the route and no broad MIR, target/codegen, ABI assignment, aggregate transport, or prealloc production consumer was switched. Prepared call plans and ABI-bound publication-routing reads remain consumer-facing or oracle surfaces. | Oracle tests cover semantic call argument/result source facts, direct-global dependency, eligible memory/publication source facts, and ABI-bound exclusions; broad backend logs stayed at `179/179`. | Do not contract call-plan or publication-routing prepared surfaces until specific MIR/codegen consumers switch one argument/result class at a time. ABI register/stack placement, outgoing stack sizing, variadic FPR count, clobbers, byval lanes, scratch, destination homes, helper/carrier protocols, aggregate transport layout, and ABI/layout-bound source-selection reads remain blockers. |
| 7. Comparison and materialized-condition producer identity | Phase A accepted comparison producer identity, branch condition provenance, lhs/rhs producer or integer constant records, `BinaryInst` condition value, producer instruction index, fused-operand provenance, and materialized-condition provenance. Phase B classified these as instruction/terminator/value annotations and rejected target branch policy. | `ComparisonProducerKind`, `ComparisonOperandProducer`, `FusedCompareOperandProducerFacts`, `MaterializedConditionProducerIdentity`, `Route7ComparisonStatus`, `Route7ComparisonOperandRole`, `Route7BranchConditionKind`, `Route7ComparisonOperandRecord`, `Route7ComparisonInstructionRecord`, `Route7BranchConditionRecord`, `Route7ComparisonConditionIndex`, route7 validation helpers, and `route_index_validate_materialized_condition_reference` through the facade. | `bir::find_materialized_condition_producer_identity(...)` moved to Route 7 records/index helpers. Future consumers must still exclude target branch spelling, fused-compare legality/can-fuse policy, condition-code selection, hazards, emitted-register state, and final instruction records/order. Prepared comparison helpers remain migration oracles for unexpanded comparison/branch consumers. | Oracle tests cover fused-operand and materialized-condition answers; reviewer found no blockers; broad backend logs stayed at `179/179`. Route index facade validates selected Route 7 references. | Do not privatize fused-compare/materialized-condition prepared surfaces until branch/comparison consumers no longer need them as oracles. Do not move target branch spelling, fused-compare legality, condition-code selection, hazard handling, emitted-register state, or final record order into BIR. |
| Route index reference facade | Phase B identified cheap lookup by stable BIR ids as required, but rejected a BIR-owned `PreparedFunctionLookups` clone. Idea 166 scoped indexes to references over typed annotations and required rebuild/validation expectations. | `RouteIndexRoute`, `RouteIndexOwnerScope`, `RouteIndexRecordCategory`, `RouteIndexRelationshipKind`, `RouteIndexValidationStatus`, `RouteIndexRecordReference`, `RouteIndexReferenceFacade`, `Route4IndexReferenceValidation`, `Route7IndexReferenceValidation`, `route_index_reference_facade`, `route_index_validate_current_block_publication_reference`, `route_index_validate_block_entry_publication_reference`, and `route_index_validate_materialized_condition_reference`. | Facade coverage is intentionally partial: selected Route 4 and Route 7 references only. Routes 1, 2, 3, 5, and 6 retain existing typed record/index shapes, so Phase C should not treat the facade as a universal contraction point. | Closure notes cite focused fail-closed validation coverage, Route 4 and Route 7 reference validation, and broad backend logs at `179/179`. | Do not replace typed route indexes with a new BIR lowering-plan aggregate. Do not make consumers scan broad BIR structures. Do not duplicate semantic payloads in index records when typed annotations already own them. |

## Step 1 Contraction Readiness Read

The Phase A/B prerequisite chain is complete enough for Step 2 inventory:

- Routes 1, 2, and 3 have implemented typed BIR records/indexes and shared MIR
  query migrations, but prepared query families still need to remain as
  migration oracles until every dependent consumer group has been audited.
- Route 4 is implemented and oracle-covered, but block-entry MIR still has a
  residual older semantic PHI-scan consumer.
- Route 5 is implemented and oracle-covered, but the current-block join-source
  public helper still uses the prior semantic implementation.
- Route 6 is implemented and oracle-covered for call-use source records, but
  broad MIR/codegen/prealloc production consumers were intentionally not
  switched.
- Route 7 is implemented and oracle-covered for materialized-condition helper
  migration, but future consumer expansion must keep branch/codegen policy
  outside BIR.
- The route index facade proves reference validation for selected Route 4 and
  Route 7 migrations only; other routes still use typed record/index shapes.

Step 2 should inventory concrete prealloc surfaces against this map and classify
each as one of:

- private construction cache ready to hide after residual consumers move;
- public migration oracle that must stay visible temporarily;
- target/layout/codegen policy surface that must remain outside BIR;
- blocked consumer-facing helper awaiting a specific BIR route migration;
- aggregate facade that should thin around typed BIR annotations without
  becoming a new BIR lowering-plan container.

## Step 2 Remaining Prealloc Surface Inventory

This inventory covers the concrete prealloc lookup, plan, stack-layout, and
direct AArch64 consumer surfaces that still define public or cache-facing
contraction pressure after the owner cleanup work from ideas 137-150.

### Residual Prepared Lookup Facade

| Surface | Semantic owner | Construction-only cache role | Consumer dependency | Public header exposure | Route prerequisite |
| --- | --- | --- | --- | --- | --- |
| `PreparedFunctionLookups` and `make_prepared_function_lookups(...)` in `src/backend/prealloc/prepared_lookups.hpp/.cpp` | Aggregate per-function lookup wiring, not a semantic fact owner. Its fields now point at call, addressing, value-location, publication, select-chain/source-producer, and return-chain lookup groups. | Yes. `src/backend/mir/aarch64/codegen/traversal.cpp` constructs one stack-local aggregate per lowered function, then projects pointers into `FunctionLoweringContext`. This is a hot cache bundle and should eventually be a private pass-internal construction detail. | AArch64 `FunctionLoweringContext` still stores `prepared_lookups`, `call_plan_lookups`, `address_materialization_lookups`, `move_bundle_lookups`, and `value_home_lookups`; `alu.cpp`, `dispatch_edge_copies.cpp`, `dispatch_producers.cpp`, `memory.cpp`, `calls.cpp`, `comparison.cpp`, `fp_value_materialization.cpp`, and `dispatch_value_materialization.cpp` still read aggregate fields or fallback-build equivalent indexes. RISC-V and x86 also still name the aggregate. | Public through `prepared_lookups.hpp`; direct AArch64 public exposure remains through `src/backend/mir/aarch64/module/module.hpp`, and `alu.cpp` still includes the facade directly. | Route index prerequisite plus Routes 1-7. The aggregate can thin only after each field has either a route-owned BIR index/facade or a target-local owner. It must not become a new BIR lowering-plan container. |
| `PreparedReturnChainLookups`, `prepared_return_chain_value_key(...)`, `make_prepared_return_chain_lookups(...)`, `find_prepared_return_chain_terminal_value(...)`, and `find_prepared_return_chain_next_operand_value(...)` | Residual return-chain identity owner; idea 141 found no narrower owner comparable to call, value-location, publication, or select-chain ownership. | Yes. The maps cache terminal/next operand value names by block/instruction/value key for return-chain emission. | AArch64 `alu.cpp` uses these helpers to recover terminal and next operand homes when scalar ALU lowering emits return-chain values. | Public only through `prepared_lookups.hpp`, and therefore coupled to the aggregate facade. | Not covered by Routes 1-7 as a named Phase B route. It should remain public until a later return-chain route or target-local owner is created; do not hide it under a BIR route without a prerequisite semantic schema. |

### Domain Lookup Headers Cleaned By Ideas 137-150

| Surface | Semantic owner | Construction-only cache role | Consumer dependency | Public header exposure | Route prerequisite |
| --- | --- | --- | --- | --- | --- |
| `src/backend/prealloc/select_chain_lookups.hpp`: `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`, select-chain dependency/materialization queries, source-producer lookup builders, current-block publication consumption, same-block integer constant evaluation | Same-block producer, select-chain, direct-global dependency, and current-block publication-consumption query ownership after idea 137 and idea 144. | Mixed. `PreparedEdgePublicationSourceProducerLookups` is a construction cache; same-block and select-chain query helpers are public oracle/compatibility facades over semantic scans and lookup caches. | AArch64 `dispatch_producers.cpp`, `calls.cpp`, `memory.cpp`, `comparison.cpp`, `fp_value_materialization.cpp`, and `dispatch_value_materialization.cpp` still build or read source-producer lookups for scalar materialization, indirect callees, call arguments, memory/store-source recovery, and comparison materialization. | Public through the narrow select-chain owner. `prepared_lookups.hpp` includes it only for aggregate field type exposure. | Routes 1 and 2 keep these public as migration oracles until same-block producer, integer constant, select-chain, and direct-global consumers finish moving to BIR records/indexes. Current-block publication consumption also depends on Route 4 coverage. |
| `src/backend/prealloc/addressing.hpp`: `PreparedAddress`, `PreparedMemoryAccess`, `PreparedAddressMaterialization`, address/materialization and memory-access lookup builders/finders, same-block global-load access, same-block load-local stored-value source | Addressing and memory-access ownership after idea 139 and idea 148. | Mixed. Address/materialization and memory-access maps are construction caches; `PreparedAddress` and `PreparedMemoryAccess` are target/layout policy records used directly by AArch64 memory and call lowering. | AArch64 `globals.cpp`, `memory.cpp`, `memory_store_retargeting.cpp`, `calls.cpp`, and tests still use address materialization lookups, frame-address offset helpers, memory-access records, load-local stored-value recovery, TLS/global relocation fields, stack slots, offsets, and base-plus-offset legality. | Public through `addressing.hpp`; `PreparedFunctionLookups` exposes `address_materializations` and `memory_accesses` as aggregate fields. | Route 3 covers semantic memory/access identity, same-block global load access, and load-local stored-value source. Target/layout fields remain outside BIR, so only pure semantic lookup/cache parts become privatization candidates after consumer migration. |
| `src/backend/prealloc/value_locations.hpp`: `PreparedValueHomeLookups`, `PreparedMoveBundleLookups`, value-home/id lookup helpers, move-bundle indexes, current-block/block-entry publication helpers | Value-location, home, move-bundle, and value publication location ownership after idea 142. | Yes for reverse indexes over homes and move bundles; no for value homes and move records themselves, which are target/codegen allocation state. | AArch64 context projects `value_home_lookups` and `move_bundle_lookups`; `memory.cpp`, `calls.cpp`, `dispatch_producers.cpp`, `dispatch_publication.cpp`, `prologue.cpp`, `select_materialization.cpp`, `operands.cpp`, and `alu.cpp` still query homes, ids, move bundles, after-call lanes, before-return moves, and publication availability. | Public through `value_locations.hpp`; aggregate fields remain exposed through `PreparedFunctionLookups`. | Routes 1, 4, and 5 can absorb only semantic producer/publication identity. Homes, move phases, execution sites, register/stack storage, and final move records are target/codegen policy and stay outside BIR. |
| `src/backend/prealloc/publication_plans.hpp`: `PreparedEdgePublicationLookups`, edge source facts, current-block join parallel-copy facts, aggregate/typed stack-source publication, scalar/store-source publication plans, edge publication source-producer records | Publication and edge-copy fact ownership after ideas 140, 145, and 150. | Mixed. Edge-publication lookup maps are construction caches; publication plans/facts still encode target-facing move/publication policy and stack-source handling. | AArch64 `dispatch_edge_copies.cpp` reads `prepared_lookups->edge_publications` for block-entry edge source facts and redundant move decisions; `dispatch_producers.cpp` fallback-builds edge publication lookups for current-block join routing; `memory.cpp` and `calls.cpp` consume edge publications and store-source publication plans. | Public through `publication_plans.hpp`; `PreparedFunctionLookups` exposes `edge_publications` and `edge_publication_source_producers`. | Routes 4 and 5 cover publication identity and join-source identity. Public exposure remains blocked by the Route 4 block-entry residual and Route 5 current-block join-source residual. Stack-source extension, hook kind, move order, and publication emission policy remain outside BIR. |
| `src/backend/prealloc/calls.hpp`: `PreparedCallPlanLookups`, call-plan records, argument/result plans, source-selection, publication-source routing, preserved-value indexes, call-boundary effects, aggregate transport and memory return plans | Call-boundary and ABI planning ownership after idea 138 and idea 146. | Mixed. `PreparedCallPlanLookups` is a construction cache; call plans carry ABI/layout policy and target move effects. | AArch64 `calls.cpp` and `calls.hpp` still depend on indexed call plans, argument plans, preserved values, outgoing stack area, before/after call move bundles, source-producer materialization, direct-global select-chain dependency, publication-source routing, memory-return handling, variadic helpers, and aggregate transport. | Public through `calls.hpp`; aggregate exposes `call_plans`. | Route 6 covers semantic call-use source records only. ABI register/stack placement, clobbers, outgoing stack size, byval lanes, variadic FPR count, helper/carrier protocols, aggregate transport, and publication routing keep call plans public and out of BIR. |
| `src/backend/prealloc/comparison.hpp`: fused-compare operand producers/facts and materialized-condition producer helper | Comparison fact ownership after idea 147. | No central lookup cache here; public helpers are compatibility/oracle queries over source-producer and constant facts. | AArch64 `comparison.cpp` still calls comparison helpers and source-producer lookup fallback paths for fused compare and materialized condition lowering. | Public through `comparison.hpp`. | Route 7 keeps these public as migration oracles until branch/comparison consumers no longer need prepared answers. Target branch spelling, fused-compare legality, condition-code selection, hazards, emitted-register state, and final record order remain blockers. |

### Module And Plan Aggregates

| Surface | Semantic owner | Construction-only cache role | Consumer dependency | Public header exposure | Route prerequisite |
| --- | --- | --- | --- | --- | --- |
| `PreparedBirModule` in `src/backend/prealloc/module.hpp` | Whole prepared-module aggregate and phase product container. | It is the phase artifact, not only a cache. It owns target profile, BIR module, names, control flow, value locations, stack layout, addressing, liveness, regalloc, frame/dynamic stack/call/publication/storage/variadic plans, carriers, runtime helpers, atomics, intrinsics, inline asm, notes, and completed phases. | AArch64 lowering receives `PreparedBirModule` and projects many domain products into `FunctionLoweringContext`; prealloc population passes mutate it phase by phase. | Public through `module.hpp`; nearly every prealloc domain header is transitively visible there. | Routes 1-7 do not authorize privatizing the whole module. Later contraction should target per-domain lookup caches or pass-internal projections, not the durable prepared artifact. |
| Plan population entrypoints: `populate_call_plans`, `populate_frame_plan`, `populate_dynamic_stack_plan`, `populate_storage_plans`, `populate_variadic_entry_plans`, plus publication/storage plan records | Domain plan owners: call ABI, frame, dynamic stack, storage, variadic entry, and store-source publication. | Mostly phase products rather than disposable caches. Some per-function lookup wrappers over plans are construction caches. | AArch64 consumes call plans, frame plans, dynamic stack operations, storage plans, variadic helper operand homes, and store-source publication plans directly for concrete instruction emission. | Public through their narrow headers and through `PreparedBirModule` fields. | Route 6 may later reduce semantic call-source dependency, but ABI/layout plan records stay public to target lowering. Routes 3-5 may reduce semantic publication/memory dependencies only where target policy is not embedded. |

### Stack Layout Surfaces

| Surface | Semantic owner | Construction-only cache role | Consumer dependency | Public header exposure | Route prerequisite |
| --- | --- | --- | --- | --- | --- |
| `src/backend/prealloc/stack_layout/stack_layout.hpp`: `find_frame_slot_by_id`, `find_stack_object_by_id`, `PreparedFrameAddressOffset`, indexed frame-address offset helpers, object collection, coalescing hints, frame-slot assignment helpers | Stack-layout and frame-object ownership after idea 143. | Lookup helpers are cheap indexed/scanning conveniences over `PreparedStackLayout`; coalescing and assignment helpers build durable stack objects/frame slots. | AArch64 `globals.cpp`, `memory_store_retargeting.cpp`, `memory.cpp`, `calls.cpp`, dynamic stack and variadic planning use stack slots, object ids, frame offsets, address materialization lookups, and value-home lookups. | Public through `stack_layout/stack_layout.hpp`; included by `prepared_lookups.hpp`, `module.hpp`, and direct AArch64 memory/store-retargeting headers. | Route 3 can only cover semantic memory/access identity. Frame slots, offsets, object ids, coalescing hints, dynamic alloca layout, and address materialization offsets are target/layout surfaces and remain outside BIR. |

### Direct AArch64 Consumer Surfaces

| Consumer surface | Semantic owner dependency | Construction-only cache role | Public header exposure | Route prerequisite |
| --- | --- | --- | --- | --- |
| `src/backend/mir/aarch64/module/module.hpp` and `codegen/traversal.cpp` | Owns target lowering context projection from `PreparedBirModule` into AArch64 lowering. | Builds `PreparedFunctionLookups` once per function and stores projected lookup pointers. | Public AArch64 module context exposes `PreparedFunctionLookups*` and domain lookup pointers. | Route index plus Routes 1-7 must first replace consumers one group at a time; final contraction should hide aggregate construction inside traversal or a private AArch64 context detail. |
| `src/backend/mir/aarch64/codegen/alu.cpp` | Return-chain, same-block producer/source-producer, move-bundle, value-home, and selected comparison/constant dependencies. | Uses the aggregate and may locally construct fallback `PreparedFunctionLookups` for helper paths. | Directly includes `prepared_lookups.hpp`; reads `return_chains` and `edge_publication_source_producers`. | Route 1 and Route 7 can reduce producer/constant/comparison oracle pressure, but return-chain has no Phase B route and remains a separate blocker. |
| `dispatch_producers.cpp` and `dispatch_edge_copies.cpp` | Same-block producer, current-block join routing, edge publication, value-home, and parallel-copy publication facts. | Fallback-builds value-home and edge-publication lookup caches when context projections are missing. | Narrow owner includes are mostly in place, but code still reads aggregate edge publication/source-producer fields through AArch64 context. | Routes 4 and 5 are blockers: block-entry publication and current-block join-source residuals keep prepared publication facts public until equivalent BIR-backed consumers exist. |
| `calls.cpp`/`calls.hpp` | Call plans, source-producer materialization, direct-global select chains, load-local stored-value source, publication routing, value homes, move bundles, variadic and aggregate transport. | Uses call-plan lookup cache plus fallback source-producer lookup construction. | Public AArch64 call APIs expose `PreparedCallPlan` and call-boundary move types from `calls.hpp`. | Route 6 only covers semantic call-use source facts. ABI/layout and aggregate transport keep call plans public; Routes 1-3 provide prerequisites for selected source fact migration. |
| `memory.cpp`, `memory.hpp`, `globals.cpp`, and `memory_store_retargeting.*` | Addressing, memory access, frame-address offsets, stack layout, value homes, edge publications, storage plans, and store-source publication. | Uses address/materialization and value-home lookup caches; some helpers construct local value-home lookups when context lacks them. | Public AArch64 memory APIs accept `PreparedAddressingFunction`, `PreparedMemoryAccess`, `PreparedStackLayout`, `PreparedAddressMaterializationLookups`, and `PreparedValueHomeLookups`. | Route 3 can reduce semantic memory/source identity, but target address materialization, frame offset, relocation/TLS, and stack-layout data keep these surfaces public. |
| `comparison.cpp` and branch/condition dispatch helpers | Fused-compare producers, materialized-condition producers, source-producer lookups, and value homes. | Uses source-producer lookup caches as compatibility/oracle inputs. | Public comparison helpers live in `comparison.hpp`; AArch64 comparison lowering still consumes prepared facts. | Route 7 must cover remaining comparison/branch semantic consumers before prepared comparison helpers can become oracle-only/private. |

### Step 2 Readiness Notes

- Ideas 137-150 successfully moved most declaration ownership out of
  `prepared_lookups.hpp`; the residual public facade pressure is now primarily
  `PreparedFunctionLookups`, return-chain helpers, and consumers that still
  take aggregate/domain lookup pointers through AArch64 lowering context.
- The most promising later privatization group is the per-function aggregate
  cache construction in AArch64 traversal, but only after direct field consumers
  have route-owned replacements or narrower private context projections.
- Return-chain lookup ownership is still ambiguous and has no Route 1-7
  prerequisite. It should be treated as a separate follow-up, not silently
  folded into producer identity or comparison work.
- Stack layout, call plans, address materialization, value homes, move bundles,
  and ABI/publication emission plans contain target/layout/codegen policy; Phase
  C should only propose contraction for semantic lookup caches layered around
  them, not for the durable target records themselves.

## Step 3 Phase B Route Contraction Status

This classification treats the Phase B route annotations as the only acceptable
semantic prerequisite for privatizing prepared lookup/cache/API families. A
family is private-ready only when BIR owns the target-neutral fact and no listed
consumer still needs the prepared API as a migration oracle. Target/layout and
codegen-policy records are not contraction targets for Phase C.

| Route or family | Prepared lookup/cache/API family | Cache contraction status | Concrete blockers | Prepared disposition | Required BIR annotation prerequisites before privatization | Public surfaces that must remain temporarily |
| --- | --- | --- | --- | --- | --- | --- |
| Route 1. Producer/source identity foundation | Same-block scalar producer, immediate integer constant, source-producer materialization, and producer-instruction identity helpers from the select-chain/source-producer lookup family. | Partially contracted at the semantic query layer; not private-ready as a prepared family. | Higher-route consumers and AArch64 materialization paths still use prepared producer/constant answers as compatibility inputs and migration oracles. | Oracle-only for producer/constant semantics; blocked for any prepared helper still feeding target lowering; out-of-scope for homes, registers, storage, emitted-register state, operand views, frame slots, and final instruction order. | Consumers must read `Route1ProducerIndex`/same-block producer and constant-query records directly or through a thin BIR-backed facade, with equivalent negative coverage for unavailable producers and constants. | `PreparedSameBlockScalarProducer`, `PreparedSameBlockValueMaterializationQuery`, same-block integer constant evaluation, source-producer lookup builders/finders, and aggregate fields exposing source-producer caches. |
| Route 2. Select-chain and direct-global dependency identity | Select-chain root/direct-global dependency and scalar materialization eligibility lookup helpers. | BIR route exists and one shared MIR query is migrated; prepared family remains oracle-only. | Call, publication, scalar materialization, and source-producer users have not all been audited or switched; prepared answers still prove direct-global, non-select-root, no-dependency, and scalar-eligibility behavior. | Oracle-only until all select-chain/direct-global consumers move; blocked where helpers still carry materialization or publication context; out-of-scope for materialization cost, hazards, registers, publication routing, call ABI, and final move/branch choices. | All consumers must rely on `Route2SelectChainValueIndex` records for accepted select roots, direct globals, root producers, and scalar eligibility, with explicit BIR fail-closed answers for rejects. | Select-chain dependency/materialization queries in `select_chain_lookups.hpp`, source-producer lookup fallback paths, and `PreparedFunctionLookups` fields that expose them. |
| Route 3. Memory/access semantic identity | Memory access identity, same-block global-load access, load-local stored-value source, address/materialization lookup caches, and public `PreparedAddress`/`PreparedMemoryAccess` records. | Semantic memory/access lookups are BIR-backed, but the prepared family is split: semantic caches are blocked-private, target address records are out-of-scope. | AArch64 memory, call, global, and store-retargeting paths still require frame slots, offsets, relocation/TLS fields, base-plus-offset legality, stack objects, and operand formation. | Blocked for pure semantic memory/source lookup caches until consumers switch; public/private-ready only for future thin semantic caches after migration; out-of-scope target policy for address materialization, layout, relocation, TLS, and stack offset records. | Consumers must use `Route3MemoryAccessIndex` for direct access identity, same-block global-load access, and load-local source facts without importing `PreparedAddress` or target addressing payloads into BIR. | `addressing.hpp` records/builders/finders, `PreparedAddressMaterializationLookups`, `PreparedMemoryAccess`, same-block access/source helpers, stack-layout offset helpers, and AArch64 memory API parameters. |
| Route 4. Block-entry and current-block publication identity | Current-block/block-entry publication availability helpers and publication value/source lookup caches. | Current-block semantic query is partially contracted; block-entry keeps the route blocked. | The block-entry MIR consumer still uses the older semantic PHI scan even though Route 4 block-entry records and indexes exist. Publication emission records also include target policy. | Oracle-only for prepared current-block and block-entry publication reads; blocked for block-entry helper privatization; out-of-scope for hook kind, destination home, storage encoding, stack-source extension, register-view conversion, immediate spelling, emitted storage, and scalar publication emission policy. | Residual block-entry MIR consumer must switch to `Route4PublicationAvailabilityIndex` or a thin BIR-backed facade, with validation for current-block and block-entry references. | Current-block/block-entry publication helpers in `value_locations.hpp`, publication availability/publication plan helpers that compare prepared answers, and aggregate fields exposing publication caches. |
| Route 5. CFG edge publication and join-source identity | Edge publication source facts, current-block join-source identity, edge-publication lookup maps, and join/publication source-producer records. | Edge source query is partially contracted; current-block join-source keeps the route blocked. | `mir::find_bir_current_block_join_source_identity(...)` still uses the prior semantic implementation, and AArch64 edge-copy/join routing still consumes prepared publication and move facts. | Oracle-only for edge/join semantic answers; blocked for current-block join-source helpers; out-of-scope target policy for parallel-copy ordering, cycle temporary routing, execution site, carrier/phase policy, coalescing, redundancy, destination registers, storage-sharing checks, and prepared move records. | Current-block join-source consumers must move to `Route5EdgeJoinSourceIndex`; edge publication and join-source lookups need BIR-backed fail-closed validation before prepared public helpers can disappear. | `publication_plans.hpp` edge-publication lookup helpers, current-block join-source helpers, `edge_publications` and `edge_publication_source_producers` aggregate fields, and AArch64 edge-copy/publication routing inputs. |
| Route 6. Call-boundary semantic source facts | Call argument/result source facts, source-producer materialization, direct-global dependency, publication-source routing, and `PreparedCallPlanLookups`/call plan APIs. | Narrow source-producer helper is BIR-backed; call family is not private-ready. | Closure explicitly left broad MIR, target/codegen, ABI assignment, aggregate transport, and prealloc production consumers unswitched. Call plans still encode ABI/layout and move effects. | Oracle-only for semantic call-use source facts; blocked for call-use consumers not migrated one argument/result class at a time; out-of-scope target policy for ABI register/stack placement, outgoing stack sizing, variadic FPR count, clobbers, byval lanes, scratch, destination homes, helper/carrier protocols, aggregate transport layout, and ABI/layout-bound source-selection reads. | Each call argument/result consumer class must use `Route6CallUseSourceIndex` records for source-producer, direct-global, eligible memory, publication, and lane facts while preserving BIR rejects for ABI-bound reads. | `PreparedCallPlanLookups`, `PreparedCallPlan`, argument/result plans, source-selection helpers, publication-source routing, preserved-value indexes, call-boundary move types, aggregate transport records, and public AArch64 call APIs. |
| Route 7. Comparison and materialized-condition producer identity | Fused-compare operand producer facts, materialized-condition producer identity helper, comparison source-producer fallback paths, and branch/condition helpers. | Materialized-condition semantic helper is BIR-backed; prepared comparison helpers remain oracle-only. | Branch/comparison consumers still need prepared answers for comparison lowering, and target branch policy/fused legality must not move into BIR. | Oracle-only for prepared comparison/materialized-condition answers; blocked until branch/comparison semantic consumers move; out-of-scope for target branch spelling, fused-compare legality, condition-code selection, hazards, emitted-register state, and final record order. | Remaining consumers must read `Route7ComparisonConditionIndex` records or route-index validation for materialized-condition references, with explicit coverage for lhs/rhs producer and integer-constant operands. | `comparison.hpp` public helpers, source-producer fallback inputs in AArch64 comparison lowering, and any prepared fused-compare/materialized-condition compatibility helpers. |
| Route index reference facade | `PreparedFunctionLookups` aggregate facade pressure and route reference validation for selected route records. | Partial facade only; not a universal contraction point. | Facade validates selected Route 4 and Route 7 references only; Routes 1, 2, 3, 5, and 6 still use typed route indexes, and AArch64 context still exposes aggregate/domain lookup pointers. | Private-ready only for narrow selected reference validation internals; blocked for aggregate facade contraction; out-of-scope as a replacement for typed route indexes or a BIR lowering-plan aggregate. | Additional route-specific typed reference validators would be needed before any consumer can use the facade as its only lookup boundary; typed annotations must remain the semantic owners. | `PreparedFunctionLookups`, `make_prepared_function_lookups(...)`, AArch64 `FunctionLoweringContext` lookup pointers, `module.hpp` exposure, and direct `prepared_lookups.hpp` includes such as `alu.cpp`. |
| Return-chain no-route gap | `PreparedReturnChainLookups`, return-chain value keying, terminal-value lookup, and next-operand lookup. | No Phase B route; cannot be contracted under Routes 1-7. | Step 2 found no narrower semantic owner comparable to call, value-location, publication, or select-chain ownership. AArch64 `alu.cpp` still uses the helpers to recover terminal and next operand homes. | Blocked and public until a return-chain route, target-local owner, or explicit non-BIR owner exists; not private-ready and not oracle-only for any current BIR route. | A new prerequisite schema would need to define target-neutral return-chain identity, terminal/next operand semantics, accepted/rejected cases, and ownership boundaries before privatization. | `PreparedReturnChainLookups`, `prepared_return_chain_value_key(...)`, `make_prepared_return_chain_lookups(...)`, `find_prepared_return_chain_terminal_value(...)`, `find_prepared_return_chain_next_operand_value(...)`, and `prepared_lookups.hpp` exposure. |

### Step 3 Classification Notes

- Routes 1, 2, 4, 5, 6, and 7 are best treated as public migration-oracle
  families until their residual consumers have BIR-backed replacements. They
  should not be deleted simply because typed BIR records exist.
- Route 3 is the clearest split family: semantic memory/access identity can
  move behind BIR-backed helpers later, but address materialization, stack
  layout, relocation/TLS, and base-plus-offset legality remain target policy.
- `PreparedFunctionLookups` is a private-cache candidate only as an aggregate
  construction detail. It cannot become the BIR route-index abstraction and
  cannot disappear while AArch64 context and fallback helper paths project its
  fields directly.
- Return-chain lookup helpers are the explicit no-route gap for Step 4. Treating
  them as Route 1 producer identity or Route 7 comparison provenance would
  overstate the existing Phase B schema.

## Step 4 Follow-Up Ideas And Proof Routes

These follow-up boundaries convert the Step 3 contraction classifications into
bounded implementation ideas. Each idea should stay narrow: migrate one
consumer group to an implemented BIR route/index, keep the prepared helper as a
temporary oracle while equivalence is proven, then contract only the cache/API
surface that no longer has public consumers. Target/layout/codegen policy
records are not privatization goals for these follow-ups.

| Follow-up idea | Cache/index group that may become private after migration | Public surfaces that must remain temporarily | Required BIR/index prerequisites | Consumer migration requirement | Proof-route recommendation |
| --- | --- | --- | --- | --- | --- |
| Route 1 producer and constant oracle thinning | Same-block scalar producer, immediate integer constant, same-block value-materialization query, and source-producer lookup caches currently exposed through `PreparedFunctionLookups`. | Prepared producer/constant helpers that still feed AArch64 materialization paths, higher-route source-producer fallback inputs, and aggregate fields exposing source-producer caches. Homes, registers, storage, emitted-register state, operand views, frame slots, and final order remain public/out-of-scope target data. | `Route1ProducerIndex`, same-block producer records, immediate integer constant records, materialization availability records, and fail-closed negative answers for unavailable producers/constants must be complete for the migrated consumer class. | Move one producer/constant consumer class at a time to `Route1ProducerIndex` or a thin BIR-backed facade. Keep prepared answers only as comparison oracles until the selected consumer no longer reads the prepared lookup fields. | Start with focused MIR/shared-producer or AArch64 materialization equivalence tests that compare BIR and prepared answers, then run the matching backend prepared lookup subset plus the relevant target lowering subset. |
| Route 2 select-chain/direct-global oracle thinning | Select-chain value/dependency lookup caches, scalar materialization eligibility lookup, and direct-global dependency indexes layered through select-chain/source-producer lookup fields. | Select-chain dependency/materialization queries, source-producer fallback paths, call/publication/materialization users not yet audited, and aggregate fields exposing select-chain caches. Materialization cost, hazards, register availability, publication routing, call ABI, and final move/branch choices stay public/out-of-scope. | `Route2SelectChainValueIndex` must answer accepted select roots, direct globals, root producers, scalar eligibility, and negative cases for non-select-root/no-dependency values. | Switch call, publication, scalar materialization, or source-producer consumers independently; do not delete prepared helpers until every selected user reads Route 2 records or an equivalent BIR facade. | Use oracle tests covering direct-global positive cases plus non-select-root, no-dependency, and scalar-ineligible rejects; pair with the narrow consumer subset that exercises the migrated call/publication/materialization path. |
| Route 3 semantic memory/access cache split | Pure semantic memory/access identity caches: direct memory access, same-block global-load access, and load-local stored-value source lookups. | `PreparedAddress`, `PreparedMemoryAccess`, `PreparedAddressMaterializationLookups`, stack-layout offset helpers, relocation/TLS/addressing records, frame slots, stack objects, base-plus-offset legality, and AArch64 memory API parameters. | `Route3MemoryAccessIndex` must provide direct access identity, same-block global-load access, load-local source facts, address-space/volatile bits, base-kind/value-role identity, and fail-closed rejects without embedding prepared addressing payloads. | Migrate semantic memory/source consumers in memory, call, global, or store-retargeting paths to Route 3 records while leaving target addressing/layout consumers on their existing prepared records. | Prove BIR/prepared equivalence for semantic access/source answers, then run targeted AArch64 memory/global/store-retargeting subsets. Reviewer should verify no `PreparedAddress`/stack-layout payload was copied into BIR. |
| Route 4 block-entry publication migration | Current-block and block-entry semantic publication availability caches, including publication value/source lookup records once no consumer needs prepared answers. | Block-entry prepared publication helpers, publication plan helpers that compare prepared answers, aggregate publication fields, and all target publication emission data: hook kind, destination home, storage encoding, stack-source extension, register-view conversion, immediate spelling, emitted storage, and scalar emission policy. | `Route4PublicationAvailabilityIndex` and selected route-index validation must cover current-block and block-entry records, source producer/value roles, and fail-closed missing-publication references. | Move the residual block-entry MIR consumer from the older PHI scan to Route 4 records or a thin BIR-backed facade before privatizing any block-entry prepared helper. | Use focused current-block and block-entry oracle tests plus the route-index reference validation subset; then run the MIR publication/query subset that exercises the migrated block-entry consumer. |
| Route 5 current-block join-source migration | Edge publication source and current-block join-source semantic lookup caches, including edge/join value records after consumer migration. | Current-block join-source public helpers, edge-publication lookup helpers, `edge_publications` and `edge_publication_source_producers` aggregate fields, and AArch64 edge-copy/publication routing inputs. Move bundle order, cycle temporary routing, execution site, carrier/phase policy, coalescing, redundancy, destination registers, storage-sharing checks, and prepared move records remain public/out-of-scope. | `Route5EdgeJoinSourceIndex` must cover CFG edge publication records, current-block join-source records, source/no-source cases, memory-source identity, and fail-closed join rejects. | Replace `mir::find_bir_current_block_join_source_identity(...)` and any selected join-source consumer with Route 5 records before hiding prepared current-block join-source helpers. | Prove edge publication and join-source oracle equivalence, then run the edge-copy/join/publication target subset that observes the migrated helper. Include negative no-source and missing-predecessor cases. |
| Route 6 call-use semantic source migration | Semantic call argument/result source lookup caches for source-producer, direct-global dependency, eligible memory/publication source, result, and lane facts. | `PreparedCallPlanLookups`, `PreparedCallPlan`, argument/result ABI plans, source-selection helpers, publication-source routing, preserved-value indexes, call-boundary move types, aggregate transport records, variadic/byval/clobber/scratch/destination-home data, and public AArch64 call APIs. | `Route6CallUseSourceIndex` must expose the selected argument/result class, including source-producer, direct-global, eligible memory/publication, result/lane facts, and rejects for ABI/layout-bound reads. | Migrate exactly one call argument/result class at a time. The consumer should read Route 6 semantic source records while continuing to read ABI/layout and move-effect records from prepared call plans. | Use call-source oracle tests for the selected class and ABI-bound negative cases, then run the narrow call-lowering subset for that class. Escalate to broader backend tests when public call API signatures or plan projections change. |
| Route 7 comparison/condition oracle thinning | Fused-compare operand producer facts, materialized-condition producer identity, comparison operand/source-producer lookup caches, and branch-condition semantic lookup helpers after consumer migration. | Prepared comparison/materialized-condition helpers still used by comparison lowering, source-producer fallback inputs, `comparison.hpp` public helpers, and target branch policy surfaces. Target branch spelling, fused-compare legality, condition-code selection, hazards, emitted-register state, and final record order stay public/out-of-scope. | `Route7ComparisonConditionIndex`, operand producer/constant records, branch condition records, materialized-condition identity, and route-index validation for selected materialized-condition references must cover positives and fail-closed rejects. | Move one comparison or branch semantic consumer at a time to Route 7 records or selected route-index validation before treating prepared comparison helpers as private oracle caches. | Use fused-operand/materialized-condition oracle tests with lhs/rhs producer and integer-constant coverage, then run the comparison/branch target subset. Reviewer should reject any movement of branch selection or fused legality policy into BIR. |
| Route index facade contraction | Narrow selected reference-validation internals for Route 4 and Route 7 only. | `PreparedFunctionLookups`, `make_prepared_function_lookups(...)`, AArch64 `FunctionLoweringContext` lookup pointers, `module.hpp` aggregate exposure, direct `prepared_lookups.hpp` includes, and typed route indexes for Routes 1, 2, 3, 5, and 6. | Additional route-specific typed reference validators are required before the facade can serve any new consumer boundary; typed annotations must remain the semantic owners. | Add facade users only where a route already has explicit reference validation. Do not route broad consumer migrations through a generic facade while typed route indexes are still the authoritative lookup surface. | Prove fail-closed validation for each newly supported reference category, then run the exact migrated consumer subset. No broad aggregate contraction should be accepted from facade tests alone. |
| PreparedFunctionLookups aggregate privacy | Per-function aggregate construction in AArch64 traversal and any fallback aggregate construction used only as pass-local cache wiring. | The aggregate type, builder, context lookup pointers, and `prepared_lookups.hpp` exposure must remain public until every projected field has either a route-owned replacement or a target-local owner. Return-chain is a separate blocker and must not be hidden through this idea. | Routes 1-7 consumer migrations plus any needed typed route validators must be complete for the fields being removed; target-local cache owners must exist for value-home, move-bundle, addressing, call-plan, stack-layout, and publication emission policy fields. | Remove direct field consumers group by group, replacing them with narrow BIR-backed or target-local projections. Only after no public consumer takes the aggregate should construction move behind traversal/private context setup. | Use compile proof for AArch64 module/context include contraction plus all migrated route subsets. Run broad backend regression when the aggregate type or module context projection changes. |

### Return-Chain No-Route Follow-Up

Return-chain must remain a separate no-route follow-up. It is not a Route 1
producer-identity cleanup and not a Route 7 comparison/condition cleanup.

| Follow-up idea | Current public/cache group | Why it cannot use Routes 1-7 | Required prerequisite before privatization | Proof-route recommendation |
| --- | --- | --- | --- | --- |
| Return-chain owner/schema decision | `PreparedReturnChainLookups`, `prepared_return_chain_value_key(...)`, `make_prepared_return_chain_lookups(...)`, `find_prepared_return_chain_terminal_value(...)`, and `find_prepared_return_chain_next_operand_value(...)`. | The current helpers recover terminal and next operand homes for return-chain emission in AArch64 ALU lowering. Phase A/B did not define a route for return-chain terminal/next operand semantics, and Step 2 found no narrower owner comparable to call, value-location, publication, or select-chain ownership. | A new bounded idea must decide whether return-chain identity is target-neutral BIR schema, a target-local AArch64 owner, or a deliberately public prepared API. If BIR-owned, it needs accepted/rejected cases, terminal/next operand records, key stability rules, and boundaries against homes/registers/final instruction order. | Analysis-first only: inventory the return-chain producer/consumer contract, then add oracle tests for terminal and next-operand answers before any API contraction. Reviewer should reject any patch that hides return-chain by folding it into Route 1 source-producer or Route 7 materialized-condition records. |

### Reviewer Reject Signals For Follow-Ups

- Reject any follow-up that deletes or privatizes a prepared helper while a
  listed consumer still reads it as a migration oracle.
- Reject any BIR annotation/index expansion that imports homes, registers,
  stack slots, frame offsets, ABI assignment, branch spelling, fused legality,
  move scheduling, emitted storage, final instruction order, or other
  target/codegen policy.
- Reject a generic `PreparedFunctionLookups` replacement that becomes a new BIR
  lowering-plan aggregate instead of using typed route records and narrow
  facades.
- Reject proof based only on a happy-path testcase when nearby negative cases
  such as no producer, no dependency, no publication, no source, ABI-bound
  reads, or non-materializable values are untested.
- Reject any return-chain contraction that is justified by Route 1 or Route 7
  without a new explicit return-chain ownership/schema decision.

## Step 5 Final Closure Summary

Phase C is complete as an analysis artifact. The contraction pass found no
prepared public surface that can be deleted or hidden immediately without a
follow-up consumer migration. The actionable result is a categorized handoff:
future work should migrate one semantic consumer group at a time to the
implemented BIR route records or narrow route facades, keep prepared answers as
oracles while equivalence is proven, and only then contract the now-unused cache
or API surface.

### Private-Cache Candidates

- `PreparedFunctionLookups` is the main private-cache candidate, but only as
  per-function aggregate construction and projection wiring in AArch64
  traversal or a private target context. It cannot become a BIR-owned
  lowering-plan aggregate.
- Route-specific semantic lookup caches can become private only after their
  consumers move: Route 1 producer/constant lookups, Route 2 select-chain and
  direct-global indexes, Route 3 pure memory/access identity lookups, Route 4
  publication availability lookups, Route 5 edge/join source lookups, Route 6
  call-use semantic source lookups, and Route 7 comparison/condition semantic
  lookups.
- Selected route-index facade internals are private-cache candidates only for
  the narrow Route 4 and Route 7 reference-validation categories already
  covered. The facade is not a universal replacement for typed route indexes.

### Temporarily Public Migration Oracles And Blockers

- Routes 1, 2, 4, 5, 6, and 7 should keep their prepared helper families public
  as migration oracles until all listed residual consumers read BIR-backed
  records or thin facades.
- Route 3 should keep semantic prepared answers public for comparison while
  memory/source consumers migrate, but its target address records are not
  oracle-only; they are durable target policy surfaces.
- The concrete blockers are the residual block-entry Route 4 consumer, the
  current-block join-source Route 5 helper, unswitched call argument/result
  classes for Route 6, branch/comparison consumers for Route 7, higher-route and
  materialization users for Routes 1 and 2, and direct aggregate field consumers
  in AArch64 lowering.

### Permanently Public Or Out-Of-Scope Target Policy

- Homes, registers, stack slots, frame offsets, stack objects, relocation/TLS,
  base-plus-offset legality, ABI register/stack assignment, outgoing stack
  sizing, variadic/byval/clobber/scratch details, publication emission records,
  move scheduling, carrier/phase policy, branch spelling, fused-compare
  legality, condition-code selection, hazards, emitted storage, final
  instruction records, and final instruction order remain outside BIR.
- `PreparedBirModule`, domain plan records, stack layout records, call plans,
  address materialization records, value-home records, move-bundle records, and
  public AArch64 APIs that need those target facts are not Phase C contraction
  targets. Later work may thin semantic lookup dependencies around them, but
  should not privatize the durable target products themselves.

### Blocked BIR-Route Migrations

- Route 1 needs selected producer/constant/materialization consumers moved to
  `Route1ProducerIndex` or an equivalent thin facade with negative coverage.
- Route 2 needs select-chain/direct-global users in call, publication,
  materialization, or source-producer paths migrated independently.
- Route 3 needs pure memory/source consumers migrated to
  `Route3MemoryAccessIndex` without copying prepared addressing payloads into
  BIR.
- Route 4 needs the residual block-entry publication consumer moved from the
  older semantic PHI scan to `Route4PublicationAvailabilityIndex` or a
  BIR-backed facade.
- Route 5 needs `mir::find_bir_current_block_join_source_identity(...)` and any
  selected join-source consumers moved to `Route5EdgeJoinSourceIndex`.
- Route 6 needs one call argument/result class at a time moved to
  `Route6CallUseSourceIndex` while ABI/layout records stay prepared-owned.
- Route 7 needs one comparison or branch semantic consumer at a time moved to
  `Route7ComparisonConditionIndex` or selected route-index validation.
- Route index facade expansion requires route-specific typed validators before
  it can support any new consumer boundary.

### Return-Chain No-Route Follow-Up

Return-chain lookup helpers remain blocked and public because Phase A/B did not
define a route for terminal and next-operand return-chain identity. The next
work should be a separate owner/schema decision: either define a target-neutral
BIR route with accepted/rejected cases, keep a target-local AArch64 owner, or
explicitly preserve a public prepared API. Return-chain must not be hidden by
folding it into Route 1 producer identity or Route 7 comparison provenance.

### Required BIR Annotation Prerequisites

- Every migration needs route-owned records/indexes for the selected semantic
  facts, fail-closed negative answers, and consumer-specific equivalence checks
  against prepared answers while the prepared helper remains public.
- BIR prerequisites must remain semantic: they may name producer identity,
  direct-global dependency, memory/source identity, publication availability,
  edge/join source identity, call-use source identity, comparison operands, and
  branch condition provenance, but not target layout or emission policy.
- Any facade expansion must validate stable typed route references rather than
  duplicating route payloads or creating a generic BIR lowering-plan container.

### Proof-Route Recommendations

- Start each follow-up with focused oracle-equivalence tests for the migrated
  semantic family, including negative cases such as no producer, no dependency,
  no publication, no source, ABI-bound reads, non-materializable values, and
  missing predecessor/reference cases.
- Pair oracle tests with the narrow migrated consumer subset: producer or
  materialization lowering for Routes 1 and 2, memory/global/store-retargeting
  for Route 3, publication/query subsets for Route 4, edge-copy/join subsets
  for Route 5, call-lowering subsets for Route 6, and comparison/branch subsets
  for Route 7.
- Escalate to broad backend regression when a follow-up changes public headers,
  AArch64 context projection, `PreparedFunctionLookups`, module/context include
  boundaries, call APIs, or any target-facing plan projection.

### Closure Handoff Recommendation

The active Phase C runbook appears complete for plan-owner closure review. The
artifact now separates private-cache candidates, temporary public migration
oracles and blockers, permanently public target-policy surfaces, blocked BIR
route migrations, the return-chain no-route gap, required BIR annotation
prerequisites, and proof routes. Closure should preserve the artifact as the
Phase D/E input and split future implementation into bounded follow-up ideas
instead of extending this analysis runbook.
