# Named BIR View Replacement Shape

This document proposes first-cut named C++ ownership boundaries to replace the
numbered route public model. The numbered `Route1` through `Route8` records
remain useful as migration references and compatibility builders, but the
durable public vocabulary should describe ownership: Producer, Memory,
Publication, Control, Call, and Return views.

The prepared-side boundary remains `prepare::PreparedFunctionLookups` in
`src/backend/prealloc/prepared_lookups.hpp` and
`mir::prepared::PreparedMirCoreView` in `src/backend/mir/prepared_view.hpp`.
Named BIR views should feed or validate those prepared views; they should not
turn route-numbered debug records into stable prepared state.

## Proposed View Boundaries

| View | First-cut C++ owner | Visibility | Replaces | Durable responsibility | Concrete current surfaces |
| --- | --- | --- | --- | --- | --- |
| Producer view | `bir::BirProducerView`, with block-local query records such as `BirSameBlockProducerIndex` and `BirProducerSourceIdentity`. | public | `Route1*` producer records and the producer parts of `Route2`, `Route4`, `Route5`, `Route6`, and `Route7`. | Own same-block scalar producer identity, materialization availability, produced value identity, and immediate constant evaluation. | `Route1ProducerRecord`, `Route1ProducerIndex`, `Route1SameBlockScalarProducer`, and `route1_find_same_block_scalar_producer` in `src/backend/bir/bir.hpp`; AArch64 `Route1PublicationSourceProducerView` in `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`. |
| Memory access view | `bir::BirMemoryAccessView`, with `BirMemoryAccessIndex` and `BirSameBlockMemorySourceView`. | public | `Route3*` memory records plus Route 3 memory-source fields embedded in `Route5` and `Route6`. | Own BIR memory access identity, address-base classification, result/stored value identity, same-block global-load source lookup, and local-load stored-value source lookup. | `Route3MemoryAccessRecord`, `Route3MemoryAccessIndex`, and same-block memory source records in `src/backend/bir/bir.hpp`; prepared owner `PreparedMemoryAccessLookups` in `src/backend/prealloc/addressing.hpp`; agreement consumers in `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` and RV64 `src/backend/mir/riscv/codegen/emit.hpp`. |
| Publication view | `bir::BirPublicationView`, with `BirBlockEntryPublicationView`, `BirCurrentBlockPublicationView`, and `BirEdgePublicationView`. | public for semantic publication facts; private for route-agreement status. | `Route4*`, `Route5*`, and the Route 4/Route 5 publication pieces of `Route6`. | Own raw BIR publication availability across current block, block entry, and CFG edge joins. It may feed prepared publication construction or local diagnostics, but prepared execution authority belongs to `PreparedEdgePublicationLookups`, `PreparedMoveBundleLookups`, and value homes. | `Route4PublicationAvailabilityIndex`, `Route5EdgeJoinSourceIndex`, `PreparedCurrentBlockEntryPublication`, `PreparedEdgePublicationLookups`, and `PreparedCurrentBlockJoinParallelCopySourceFact` in `src/backend/prealloc/value_locations.hpp` and `src/backend/prealloc/publication_plans.hpp`. |
| Control-flow value view | `bir::BirControlValueView`, with `BirSelectChainView` and `BirComparisonConditionView`. | public for select/comparison semantic queries; compatibility-only for old route-index validation statuses. | `Route2*` select-chain records and `Route7*` comparison records. | Own select-chain producer/dependency identity, branch comparison operands, materialized condition identity, and branch-condition reconstruction. It should expose semantic control-value facts rather than `route_index_validate_*` status records. | `Route2SelectChainValueIndex`, `Route7ComparisonConditionIndex`, `Route7ComparisonInstructionRecord`, and `Route7BranchConditionRecord` in `src/backend/bir/bir.hpp`; AArch64 comparison consumers in `src/backend/mir/aarch64/codegen/comparison.cpp`; prepared comparison helpers in `src/backend/prealloc/comparison.hpp`. |
| Call boundary view | `bir::BirCallBoundaryView`, with `BirCallArgumentSourceView` and `BirCallResultSourceView`. | public for BIR call source/result facts; private for agreement-only publication fallback details. | `Route6*` call argument/result records. | Own call argument source encoding, direct-global argument dependency, publication/memory fallback source, result value identity, and result-lane identity. Prepared call placement and ABI effects remain in `PreparedCallPlanLookups`. | `Route6CallUseSourceIndex`, `Route6CallArgumentSourceRecord`, `Route6CallArgumentPublicationSourceRecord`, and `Route6CallResultLaneSourceRecord` in `src/backend/bir/bir.hpp`; prepared owner `PreparedCallPlanLookups` in `src/backend/prealloc/calls.hpp`; AArch64 call consumers in `src/backend/mir/aarch64/codegen/calls.cpp`. |
| Return-chain view | `bir::BirReturnChainView`, with `BirReturnChainIndex` and `BirReturnChainValueKey`. | public only if target-independent return-chain lowering still needs it; otherwise compatibility-only. | `Route8*` return-chain records. | Own return-chain terminal value and next-operand value identity for scalar publication chains that end at a return. This should not become a general value-home authority. | `Route8ReturnChainIndex`, `Route8ReturnChainRecord`, `route8_find_return_chain_terminal_value`, and `route8_find_return_chain_next_operand_value` in `src/backend/bir/bir.hpp`; AArch64 ALU consumers in `src/backend/mir/aarch64/codegen/alu.cpp`. |

## Public, Private, And Compatibility Layers

The replacement should have three layers.

| Layer | Intended C++ shape | Scope |
| --- | --- | --- |
| Public semantic views | `BirProducerView`, `BirMemoryAccessView`, `BirPublicationView`, `BirControlValueView`, `BirCallBoundaryView`, and possibly `BirReturnChainView`. | Stable BIR-side query vocabulary for prepared builders, target lowering, and diagnostics. Names are ownership-based, not route-numbered. |
| Private builder indexes | Block/function-local builder structs such as `BirSameBlockProducerIndex`, `BirMemoryAccessIndex`, `BirPublicationAvailabilityIndex`, `BirComparisonConditionIndex`, and `BirCallUseSourceIndex`. | Implementation detail under `src/backend/bir/`. These can reuse current route algorithms while their public names stop exposing `Route1` through `Route8`. |
| Compatibility adapters | Thin wrappers with route-numbered names and route-index status records. | Temporary migration surface for existing consumers and tests. `RouteIndexReferenceFacade`, `Route4IndexReferenceValidation`, and `Route7IndexReferenceValidation` should live here until replaced by named proof views. |

This keeps `route1..8` from becoming the durable public model. Route numbers
can remain in migration comments, test fixture names, or temporary adapter
functions while consumers move to named public views.

## Current Route Mapping

| Current route record family | Named destination | Visibility after migration | Notes |
| --- | --- | --- | --- |
| `Route1SourceValueIdentity`, `Route1ProducerRecord`, `Route1ProducerIndex`, `Route1SameBlockScalarProducer`, `Route1MaterializationAvailability`, `Route1ImmediateIntegerConstant` | `BirProducerView` / `BirSameBlockProducerIndex` | public semantic view, private index | Producer identity is the shared substrate for memory, publication, call, comparison, and return views. |
| `Route2SelectChainProducerRecord`, `Route2SelectChainDirectGlobalDependencyRecord`, `Route2SelectChainValueRecord`, `Route2SelectChainValueIndex` | `BirControlValueView::SelectChain` or `BirSelectChainView` | public semantic view | Select-chain facts are control/value-shape facts. Direct-global dependency can also feed call-boundary argument classification. |
| `Route3MemoryAccessRecord`, `Route3MemoryAccessValueRecord`, `Route3MemoryAccessIndex`, same-block load/global source records | `BirMemoryAccessView` | public semantic view | Memory access identity is a real semantic input. Route-numbered agreement with prepared memory data should be private diagnostic state. |
| `Route4CurrentBlockPublicationRecord`, `Route4BlockEntryPublicationRecord`, `Route4PublicationValueRecord`, `Route4PublicationAvailabilityIndex` | `BirPublicationView::CurrentBlock` and `BirPublicationView::BlockEntry` | public semantic facts; compatibility-only validation status | Publication facts are useful, but `Route4IndexReferenceValidation` and route status fields on `PreparedCurrentBlockEntryPublication` are compatibility/proof residue. |
| `Route5CfgEdgePublicationRecord`, `Route5CurrentBlockJoinSourceRecord`, `Route5PublicationValueRecord`, `Route5EdgeJoinSourceIndex` | `BirPublicationView::CfgEdge` and prepared `PreparedEdgePublicationLookups` agreement probes | public for raw BIR publication facts; private for stored agreement annotations | Executable edge publication authority is prepared-owned. Route 5 agreement fields on `PreparedCurrentBlockJoinParallelCopySourceFact` should not be durable public state. |
| `Route6CallArgumentSourceRecord`, producer/direct-global/publication-source records, `Route6CallResultSourceRecord`, result-lane records, `Route6CallUseSourceIndex` | `BirCallBoundaryView` | public semantic view | The call boundary view may reference Producer, Memory, Publication, and Control views, but should present call-specific source/result records. |
| `Route7ComparisonInstructionRecord`, `Route7ComparisonOperandRecord`, `Route7BranchConditionRecord`, `Route7ComparisonConditionIndex` | `BirControlValueView::Comparison` or `BirComparisonConditionView` | public semantic view; compatibility-only facade validation | AArch64 comparison lowering currently uses Route 7 both semantically and as agreement proof. The route-index validation status should move to a named proof adapter. |
| `Route8ReturnChainValueKey`, `Route8ReturnChainRecord`, `Route8ReturnChainIndex` | `BirReturnChainView` | public if retained across targets; otherwise compatibility-only | Return-chain facts are target-lowering helpers today. Keep the first extraction narrow and avoid treating them as value-home authority. |
| `RouteIndexRoute`, `RouteIndexRecordReference`, `RouteIndexReferenceFacade`, `Route4IndexReferenceValidation`, `Route7IndexReferenceValidation` | `BirViewCompatibilityProof` or route-specific proof adapters | compatibility-only | These records encode validation status for Route 4 and Route 7 only. They should not be the named public replacement. |

## Prepared Boundary Mapping

Named BIR views should stop at the BIR/prepared boundary.
`PreparedFunctionLookups` already aggregates prepared-owned codegen facts:
`PreparedCallPlanLookups`, `PreparedAddressMaterializationLookups`,
`PreparedMemoryAccessLookups`, `PreparedMoveBundleLookups`,
`PreparedValueHomeLookups`, `PreparedEdgePublicationLookups`,
`PreparedEdgePublicationSourceProducerLookups`, and branch stack-load
authority records. `PreparedMirCoreView` then exposes those lookups with BIR
function/block binding through `PreparedMirFunctionEntry` and
`PreparedMirFunctionView`.

The replacement shape should use this split:

| Prepared area | Named BIR view allowed to feed or validate it | Public authority |
| --- | --- | --- |
| Value homes and move bundles | Producer and Publication views can validate source/destination identities. | `PreparedValueHomeLookups` and `PreparedMoveBundleLookups` are public prepared authority. |
| Edge publications and join transfers | Publication, Producer, and Memory views can help construct or diagnose source facts. | `PreparedEdgePublicationLookups` and `PreparedCurrentBlockJoinParallelCopySourceFact` are public prepared authority, minus route-numbered agreement fields. |
| Memory accesses | Memory view can validate BIR access identity. | `PreparedMemoryAccessLookups` is public prepared authority for codegen. |
| Calls | Call boundary view can validate source/result identity. | `PreparedCallPlanLookups` is public prepared authority for ABI and call-boundary effects. |
| Branch comparisons | Control-flow value view can validate comparison producers and materialized conditions. | Prepared comparison/branch facts and target lowering own codegen decisions; route-index status is compatibility-only. |
| Return chain | Return-chain view may provide target-lowering evidence. | Prepared value homes and return ABI moves remain authority; return-chain view is not stack or home authority. |

## Migration Shape

1. Introduce named view aliases or wrappers in `src/backend/bir/` that forward
   to the current route builders without changing behavior. The first
   implementation should prefer compatibility adapters over broad rewrites.
2. Move consumers that need semantic facts to named views first. Good early
   candidates are local proof readers around Route 4 block-entry attribution
   in `src/backend/prealloc/prepared_lookups.cpp` and Route 7 comparison
   validation in `src/backend/mir/aarch64/codegen/comparison.cpp`, because
   those are already facade-shaped.
3. Keep route-numbered status and `RouteIndexReferenceFacade` private to
   compatibility code until dump/proof policy no longer prints those names.
4. Do not add route-numbered fields to `PreparedFunctionLookups` or
   `PreparedMirCoreView`. If prepared or MIR needs a fact, name the prepared
   fact directly; if diagnostics need historical agreement, recompute it at
   the diagnostic surface.

## First-Cut C++ Naming Sketch

The following names are intentionally ownership-based and can be implemented as
thin wrappers at first:

```cpp
namespace c4c::backend::bir {

struct BirProducerView;
struct BirSameBlockProducerIndex;
struct BirMemoryAccessView;
struct BirPublicationView;
struct BirControlValueView;
struct BirCallBoundaryView;
struct BirReturnChainView;

struct BirCompatibilityProofView;

}  // namespace c4c::backend::bir
```

`BirCompatibilityProofView` is the only place where route-numbered validation
status should survive as a public-looking type during migration. Long term,
proof surfaces should use named relationships such as publication agreement,
comparison operand agreement, and materialized condition agreement rather than
`Route4` or `Route7` status vocabulary.

## Non-Authority Rules

- Producer, Memory, Publication, Control, Call, and Return views describe BIR
  facts. They do not override prepared value homes, move bundles, call plans,
  or prepared MIR core bindings.
- `RouteIndexReferenceFacade` is compatibility-only. It should not grow into a
  registry for Route 1 through Route 8.
- Route 4, Route 5, and Route 7 agreement annotations are diagnostic/proof
  facts unless a later implementation plan promotes a named prepared fact with
  explicit ownership.
- Return-chain facts must not become implicit destination authority for stack
  or value-home decisions.
