# Current BIR Route Ownership Inventory

This document inventories the current `bir_route_index`-adjacent route surface
before any retirement design work. It classifies the existing route files and
public record families by the role they play today:

- producer lookup: reconstructs BIR-local producer/source facts.
- semantic view: exposes a higher-level BIR relationship for prepared/MIR use.
- prealloc input: feeds prepared/prealloc planning or prepared lookup
  attribution.
- diagnostic/proof artifact: validates or prints agreement between BIR route
  facts and prepared facts.
- compatibility residue: preserves an older raw-BIR route API, facade, spelling,
  or status surface that is not the semantic owner long term.

## Route File Inventory

| File | Route surface | Primary current role | Public outputs | Direct consumers observed |
| --- | --- | --- | --- | --- |
| `src/backend/bir/bir_route1.cpp` | Route 1 same-block scalar producer identity and immediate evaluation. | producer lookup | `Route1SourceValueIdentity`, `Route1ProducerRecord`, `Route1ProducerIndex`, `Route1SameBlockScalarProducer`, `Route1MaterializationAvailability`, `Route1ImmediateIntegerConstant` | Route 2, Route 4, Route 5, Route 6, Route 7, AArch64 dispatch producers, AArch64 calls, AArch64/RV64 prepared materialization checks. |
| `src/backend/bir/bir_route2.cpp` | Route 2 select-chain and direct-global dependency reconstruction. | semantic view | `Route2SelectChainProducerRecord`, `Route2SelectChainDirectGlobalDependencyRecord`, `Route2SelectChainValueRecord`, `Route2SelectChainValueIndex` | Route 6 direct-global call-argument dependency, AArch64 dispatch value materialization, AArch64 ALU/control select-chain materialization. |
| `src/backend/bir/bir_route3_memory.cpp` | Route 3 BIR memory access identity and same-block memory-source lookups. | semantic view / producer lookup | `Route3MemoryAccessRecord`, `Route3MemoryAccessValueRecord`, `Route3MemoryAccessIndex`, `Route3SameBlockGlobalLoadAccessRecord`, `Route3SameBlockLoadLocalSourceRecord`, `Route3SameBlockLoadLocalStoredValueSourceRecord` | Route 5 memory-source edge publications, Route 6 publication-source fallback, AArch64 globals/ALU/calls memory agreement, RV64 prepared edge publication agreement. |
| `src/backend/bir/bir_route4_publication.cpp` | Route 4 current-block and block-entry publication availability. | prealloc input / diagnostic/proof artifact | `Route4CurrentBlockPublicationRecord`, `Route4BlockEntryPublicationRecord`, `Route4PublicationValueRecord`, `Route4PublicationAvailabilityIndex`, route-index validation records | `src/backend/prealloc/prepared_lookups.cpp` block-entry attribution, `src/backend/prealloc/prepared_printer/value_locations.cpp` proof rows, AArch64 dispatch publication/calls current-block source agreement. |
| `src/backend/bir/bir_route5_publication.cpp` | Route 5 CFG edge publication and current-block join-source reconstruction. | prealloc input / semantic view | `Route5CfgEdgePublicationRecord`, `Route5CurrentBlockJoinSourceRecord`, `Route5PublicationValueRecord`, `Route5EdgeJoinSourceIndex` | `src/backend/prealloc/publication_plans.cpp` current-block join-source agreement, prepared printer select-chain diagnostics, AArch64 dispatch producers, RV64 prepared edge publication emission. |
| `src/backend/bir/bir_route6_call_publication.cpp` | Route 6 call argument/result source reconstruction. | semantic view / producer lookup | `Route6CallArgumentSourceRecord`, `Route6CallArgumentSourceProducerRecord`, `Route6CallArgumentDirectGlobalDependencyRecord`, `Route6CallArgumentPublicationSourceRecord`, `Route6CallResultSourceRecord`, `Route6CallResultLaneSourceRecord`, `Route6CallUseSourceIndex` | AArch64 dispatch builds a lazy `Route6CallUseSourceIndex`; AArch64 calls uses route6 argument-source and result-source agreement; call-plan publication has route6-named scalar source diagnostics. |
| `src/backend/bir/bir_route7_comparison.cpp` | Route 7 comparison instruction, operand producer, and branch-condition reconstruction. | semantic view / diagnostic/proof artifact | `Route7ComparisonInstructionRecord`, `Route7ComparisonOperandRecord`, `Route7BranchConditionRecord`, `Route7ComparisonConditionIndex`, route-index validation records | AArch64 comparison lowering validates fused compare operand producer facts and materialized-condition identities through the `bir_route_index` facade. |
| `src/backend/bir/bir_route8.cpp` | Route 8 return-chain value identity. | semantic view / compatibility residue | `Route8ReturnChainValueKey`, `Route8ReturnChainRecord`, `Route8ReturnChainIndex` | AArch64 ALU return-chain value-home lookup and return-chain next-operand/terminal-value helpers. |
| `src/backend/bir/bir_route_facade.cpp` | Narrow `bir_route_index` facade over Route 4 and Route 7 indexes. | compatibility residue / diagnostic/proof artifact | `RouteIndexReferenceFacade`, `route_index_reference_facade(...)`, `route_index_validate_*` wrappers | Prealloc Route 4 block-entry attribution, AArch64 Route 7 materialized-condition and operand-reference validation. |

## Public Route-Index Record Families

| Family | Declared in | Classification | Notes |
| --- | --- | --- | --- |
| `RouteIndexRoute` | `src/backend/bir/bir_route_index.hpp` | compatibility residue | Enumerates only `Route4PublicationAvailability` and `Route7ComparisonCondition`; it is not a general route registry for bir_route1 through bir_route8. |
| `RouteIndexOwnerScope` | `src/backend/bir/bir_route_index.hpp` | compatibility residue | Carries function/block ownership metadata for validation references. |
| `RouteIndexRecordCategory` | `src/backend/bir/bir_route_index.hpp` | diagnostic/proof artifact | Names only Route 4 current/block-entry publication and Route 7 comparison/operand/branch records. |
| `RouteIndexRelationshipKind` | `src/backend/bir/bir_route_index.hpp` | diagnostic/proof artifact | Captures the expected relationship checked by validation, including Route 7 materialized-condition references. |
| `RouteIndexValidationStatus` | `src/backend/bir/bir_route_index.hpp` | diagnostic/proof artifact | Shared status vocabulary for route-reference agreement, divergence, duplicate, stale-owner, and missing-record cases. |
| `RouteIndexRecordReference` | `src/backend/bir/bir_route_index.hpp` | diagnostic/proof artifact | Structured coordinate for a validation target; embeds Route 1 value identity and Route 7 operand role. |
| `RouteIndexReferenceFacade` | `src/backend/bir/bir_route_index.hpp`, implemented in `bir_route_facade.cpp` | compatibility residue | Optional holder for Route 4 and Route 7 indexes only. Retirement should treat this as a wrapper, not the semantic source. |
| `Route4IndexReferenceValidation` | `src/backend/bir/bir_route_index.hpp`, implemented in `bir_route4_publication.cpp`/`bir_route_facade.cpp` | diagnostic/proof artifact / prealloc input | Carries route-index status plus direct pointers to current-block or block-entry Route 4 records. Currently feeds prealloc block-entry attribution. |
| `Route7IndexReferenceValidation` | `src/backend/bir/bir_route_index.hpp`, implemented in `bir.cpp`/`bir_route_facade.cpp` | diagnostic/proof artifact | Carries route-index status plus direct pointers to comparison, operand, or branch Route 7 records. Currently feeds AArch64 comparison agreement checks. |

## Public Route Record Families

| Route | Public record/index families | Classification | Current ownership summary |
| --- | --- | --- | --- |
| Route 1 | `Route1SourceValueIdentity`, `Route1ImmediateIntegerConstant`, `Route1ProducerInstructionIdentity`, `Route1MaterializationAvailability`, `Route1ProducerRecord`, `Route1ProducerIndex`, `Route1SameBlockProducerQuery`, `Route1SameBlockScalarProducer` | producer lookup | Shared raw-BIR producer substrate. Other routes and target lowering still query it directly. |
| Route 2 | `Route2SelectChainProducerRecord`, `Route2SelectChainDirectGlobalDependencyRecord`, `Route2SelectChainValueRecord`, `Route2SelectChainValueIndex`, `Route2SelectChainValueQuery` | semantic view | Select-chain/direct-global dependency view layered on Route 1. Used by call argument and value materialization paths. |
| Route 3 | `Route3MemoryAccessRecord`, `Route3MemoryAccessValueRecord`, `Route3MemoryAccessIndex`, `Route3MemoryAccessQuery`, same-block load/global records | semantic view / producer lookup | Memory access identity and source lookup used to prove prepared memory-source agreement. |
| Route 4 | current-block, block-entry, value records and `Route4PublicationAvailabilityIndex` | prealloc input / diagnostic/proof artifact | Publication agreement bridge between raw BIR and prepared block-entry/current-block publication facts. |
| Route 5 | edge publication, current-block join-source, value records and `Route5EdgeJoinSourceIndex` | prealloc input / semantic view | Edge/join publication bridge used by prealloc publication plans and target prepared edge emission. |
| Route 6 | call argument source, producer, direct-global, publication-source, result, result-lane records and `Route6CallUseSourceIndex` | semantic view / producer lookup | Call boundary source/result reconstruction used mostly by AArch64 call lowering and call diagnostics. |
| Route 7 | comparison instruction, operand, branch condition records and `Route7ComparisonConditionIndex` | semantic view / diagnostic/proof artifact | Comparison agreement bridge used by AArch64 comparison lowering; Route 7 is one of only two `bir_route_index` facade members. |
| Route 8 | return-chain key, record, and index | semantic view / compatibility residue | Return-chain helper for AArch64 return-chain/ALU paths; not part of `bir_route_index`. |

## Direct Consumer Inventory

| Consumer area | Files | Route families consumed | Consumption kind |
| --- | --- | --- | --- |
| Prepared block-entry lookup attribution | `src/backend/prealloc/prepared_lookups.cpp`, `src/backend/prealloc/value_locations.hpp` | Route 4, `bir_route_index` facade/status | prealloc input / diagnostic proof. It validates prepared block-entry publication against a temporary Route 4 index and records attribution fields on `PreparedCurrentBlockEntryPublication`. |
| Prepared printer block-entry proof rows | `src/backend/prealloc/prepared_printer/value_locations.cpp` | Route 4, `bir_route_index` facade/status | diagnostic/proof artifact. It reuses the prepared lookup attribution to decide whether printed block-entry publication rows have an agreeing Route 4 source. |
| Prepared current-block join-source attribution | `src/backend/prealloc/publication_plans.cpp`, `src/backend/prealloc/publication_plans.hpp` | Route 5 | prealloc input / diagnostic proof. It attaches an agreeing `Route5CurrentBlockJoinSourceRecord` and status to prepared current-block join-source facts. |
| Prepared printer select-chain/current-block join diagnostics | `src/backend/prealloc/prepared_printer/select_chains.cpp` | Route 5 status/agreement fields | diagnostic/proof artifact. It prints `route5_status` and `route5_agrees` for prepared join-source facts. |
| Prepared lookup aggregate view | `src/backend/prealloc/prepared_lookups.hpp`, `src/backend/prealloc/prepared_lookups.cpp` | Prepared facts, plus Route 4 attribution bridge | semantic view / prealloc input. `make_prepared_function_lookups` is already the prepared lookup owner for MIR-facing prepared facts; only Route 4 attribution remains directly route-index-shaped. |
| Prepared MIR core view | `src/backend/mir/prepared_view.hpp`, `src/backend/mir/prepared_view.cpp` | Prepared prealloc lookups, not raw `bir_route_index` directly | semantic view. `PreparedMirFunctionEntry` owns `prepare::PreparedFunctionLookups`; this is the newer MIR-facing view boundary that should replace raw route-index consumers where possible. |
| AArch64 dispatch producers/publication/value materialization | `src/backend/mir/aarch64/codegen/dispatch_producers.*`, `dispatch_publication.cpp`, `dispatch_value_materialization.cpp` | Route 1, Route 2, Route 3, Route 4, Route 5 | semantic view / compatibility residue. These paths compare raw BIR route facts with prepared facts during target lowering. |
| AArch64 calls | `src/backend/mir/aarch64/codegen/calls.cpp`, `calls.hpp`, `dispatch.cpp` | Route 1, Route 3, Route 4, Route 6 | semantic view / diagnostic proof. Dispatch lazily builds Route 6 indexes; calls validates argument-source producers, indirect callee sources, and result-source register evidence. |
| AArch64 comparison | `src/backend/mir/aarch64/codegen/comparison.cpp` | Route 7, `bir_route_index` facade/status | semantic view / diagnostic proof. It validates materialized condition and fused compare operand references through the facade. |
| AArch64 globals/ALU | `src/backend/mir/aarch64/codegen/globals.cpp`, `alu.cpp` | Route 3, Route 8 | semantic view / compatibility residue. Globals checks Route 3 global-load agreement; ALU uses Route 8 return-chain helpers. |
| RV64 prepared edge publication | `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`, `emit.hpp`, `object_emission.cpp` | Route 3, Route 5 | diagnostic/proof artifact. It records Route 5 edge status/source agreement and Route 3 source-memory agreement in prepared edge publication intent diagnostics. |

## Route-Index Retirement Notes

- `bir_route_index` is currently a narrow Route 4/Route 7 validation facade, not
  the owner of all `bir_routeN` files.
- Route 1, Route 2, Route 3, Route 5, Route 6, and Route 8 are direct raw route
  APIs with independent consumers. Retiring the facade alone does not retire
  those route lookups.
- The prepared/prealloc side already has a stronger semantic aggregation point:
  `prepare::PreparedFunctionLookups` and `mir::prepared::PreparedMirCoreView`.
  Current raw-route consumers that remain in target lowering are compatibility
  bridges used to prove prepared facts agree with older BIR-route facts.
- The highest-value first retirement target is the facade/status residue around
  Route 4 block-entry attribution and Route 7 comparison validation, because
  those are the only public `bir_route_index` record families.
