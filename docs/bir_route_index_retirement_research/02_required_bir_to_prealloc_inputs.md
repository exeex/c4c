# Required BIR-To-Prealloc Inputs

This document separates the BIR facts that prealloc and prepared MIR need for
codegen from route-index facts that exist only to prove or print agreement with
older numbered routes. The boundary is `prepare::PreparedFunctionLookups` and
`mir::prepared::PreparedMirCoreView`, not the `bir_route_index` facade.

## Minimum Codegen Input Facts

| Required fact | Prepared owner | Current route-index contact | Codegen input classification | Concrete consumers |
| --- | --- | --- | --- | --- |
| Function and block identity: prepared function id, BIR function pointer, prepared block label, block index, and BIR block pointer. | `mir::prepared::PreparedMirCoreView` builds `PreparedMirFunctionEntry` and `PreparedMirBlockView` from prepared control flow plus the BIR function binding in `src/backend/mir/prepared_view.cpp`. | None required. | Required codegen input because target lowering must relate prepared blocks back to BIR instructions and terminators. | `src/backend/mir/prepared_view.hpp`, `src/backend/mir/prepared_view.cpp`, AArch64/RV64 prepared lowering entry points that receive prepared views or `PreparedFunctionLookups`. |
| Prepared value homes: value id/name, home kind, register/stack/immediate/pointer-home placement. | `prepare::PreparedValueHomeLookups` inside `prepare::PreparedFunctionLookups`; built by `make_prepared_value_home_lookups` in `src/backend/prealloc/prepared_lookups.cpp`. | None required. | Required codegen input. It is the stable answer for where a prepared value lives. | `src/backend/mir/prepared_view.cpp` direct-edge source views, RV64 prepared scalar/call/frame emitters, and prealloc publication queries. |
| Prepared edge publications: predecessor label, successor label, destination value id/name/home, source value, source home, publication move, parallel-copy metadata, and aggregate stack-source authority. | `prepare::PreparedEdgePublicationLookups` in `src/backend/prealloc/publication_plans.hpp`; built by `make_prepared_edge_publication_lookups` in `src/backend/prealloc/prepared_lookups.cpp`. | Route 5 currently attaches agreement annotations for join-source proof, but the edge-publication lookup itself is prepared-owned. | Required codegen input for out-of-SSA edge moves and direct edge publication source selection. | `prepare_current_block_join_parallel_copy_source_facts` in `src/backend/prealloc/publication_plans.cpp`; `PreparedMirFunctionView::current_block_direct_edge_publication_sources` in `src/backend/mir/prepared_view.cpp`; RV64 prepared edge publication emission. |
| Prepared source producer for an edge publication: producer kind, block label, instruction index, and producer payload for load-local/load-global/cast/binary/select materialization. | `PreparedEdgePublicationSourceProducerLookups` inside `PreparedFunctionLookups`; constructed by `make_prepared_edge_publication_source_producer_lookups`. | Route 5 has an older `Route5PublicationSourceKind` used to compare with prepared source producers. | Required codegen input when a prepared publication source must be rematerialized or tied to a BIR producer. The route spelling is not required. | `apply_source_producer_fact` and edge publication construction in `src/backend/prealloc/prepared_lookups.cpp`; AArch64 dispatch producer checks currently compare Route 5 against prepared facts. |
| Prepared memory access identity for publication sources and pointer uses: block label, instruction index, stored/result value names, object/global facts, and value-home references. | `PreparedMemoryAccessLookups` in `PreparedFunctionLookups`; built and repaired in `make_prepared_function_lookups` in `src/backend/prealloc/prepared_lookups.cpp`. | Route 3 and Route 5 currently provide agreement evidence for some memory-source publications. | Required codegen input for memory-backed publication decisions; Route 3/5 agreement status is proof/debug residue. | `apply_source_memory_access_fact` in `src/backend/prealloc/prepared_lookups.cpp`; RV64 prepared edge publication diagnostics; AArch64 global/call agreement checks. |
| Move bundle and parallel-copy execution facts: bundle phase, authority kind, predecessor/successor labels, moves, execution site, cycle temp use, and source freshness authority. | `PreparedMoveBundleLookups`, `PreparedEdgePublicationLookups`, and `PreparedValueFreshnessAuthority` records in prealloc. | Route 5 may confirm that a current-block join source matches a historical BIR publication record. | Required codegen input. The agreement pointer/status is not required for executing the move. | `prepare_current_block_join_parallel_copy_source_facts` in `src/backend/prealloc/publication_plans.cpp`; `PreparedMirFunctionView::current_block_direct_edge_publication_sources`; RV64 prepared function emission. |
| Call, address materialization, branch stack-load authority, and value freshness facts. | `PreparedFunctionLookups` fields in `src/backend/prealloc/prepared_lookups.hpp`. | Routes 1, 2, 3, 4, and 6 still have target-lowering agreement consumers, but they are not `bir_route_index` inputs. | Required codegen input where target lowering consumes prepared call/address/freshness authority. The numbered route comparisons should remain optional proof until replaced. | AArch64 calls/dispatch, RV64 prepared call/frame/scalar emitters, and prepared MIR core snapshots. |

## Proof And Debug Inputs

These facts should not be treated as required BIR-to-prealloc codegen inputs
after the proof contract moves away from intermediate route-dump comparison.

| Fact | Current surface | Why it is proof/debug, not codegen input | Current consumer |
| --- | --- | --- | --- |
| `RouteIndexReferenceFacade` over Route 4 and Route 7. | `src/backend/bir/bir_route_index.hpp` and `src/backend/bir/bir_route_facade.cpp`. | It only wraps optional Route 4/Route 7 indexes and carries validation status. It does not own prepared value placement, edge moves, or target storage decisions. | `src/backend/prealloc/prepared_lookups.cpp` for Route 4 block-entry attribution; `src/backend/mir/aarch64/codegen/comparison.cpp` for Route 7 comparison agreement. |
| Route 4 block-entry attribution fields on `PreparedCurrentBlockEntryPublication`. | `route4_block_entry_publication_attributed`, `route4_block_entry_publication_status`, `route4_block_entry_publication_route_status`, and `route4_block_entry_publication_instruction_index` in `src/backend/prealloc/value_locations.hpp`. | The prepared publication is already available through `PreparedBlockEntryPublication`, destination value id/name, destination home, and publication bundle/move. The Route 4 fields only say whether an older BIR publication record agreed. | `attribute_route4_block_entry_publication_if_agreeing` in `src/backend/prealloc/prepared_lookups.cpp`; prepared printer rows in `src/backend/prealloc/prepared_printer/value_locations.cpp`. |
| Route 5 join-source pointer/status/agreement on `PreparedCurrentBlockJoinParallelCopySourceFact`. | `route5_join_source`, `route5_join_source_status`, and `route5_join_source_agrees` in `src/backend/prealloc/publication_plans.hpp`. | The executable source fact is already described by prepared edge publication, source/destination homes, move, source freshness authority, and incoming/source-value booleans. The Route 5 fields are an agreement annotation. | `attach_route5_current_block_join_source_if_agrees` and `route5_join_source_record_agrees_with_prepared_fact` in `src/backend/prealloc/publication_plans.cpp`; printer output in `src/backend/prealloc/prepared_printer/select_chains.cpp`; AArch64 dispatch producer agreement checks. |
| Route 7 materialized-condition and operand-reference validation status. | `Route7IndexReferenceValidation` and `route_index_validate_*` wrappers in `src/backend/bir/bir_route_index.hpp`; calls in `src/backend/mir/aarch64/codegen/comparison.cpp`. | These checks validate that comparison lowering still matches older BIR comparison route records. They are not inputs to prepared prealloc ownership. | AArch64 comparison lowering. |
| Route status names printed into prepared dumps. | Route 4 status names in value-location proof rows and Route 5 status/agreement in select-chain/current-block join rows. | Dumps should be able to report prepared-owned status directly. Route-numbered status fields are observation aids. | `src/backend/prealloc/prepared_printer/value_locations.cpp` and `src/backend/prealloc/prepared_printer/select_chains.cpp`. |

## Facts To Recompute Locally

The following facts should be recomputed at the local consumer when needed for
proof, assertion, or a temporary bridge. They should not be stored as durable
route-index state in `PreparedFunctionLookups` or `PreparedMirCoreView`.

| Recomputable fact | Local inputs | Recompute surface | Reason not to store |
| --- | --- | --- | --- |
| Route 4 block-entry agreement for a single prepared block-entry publication. | Successor BIR block, destination BIR value, prepared successor label, destination value id/name, and publication bundle instruction index. | Existing local pattern in `attribute_route4_block_entry_publication_if_agreeing` builds a one-block Route 4 index and immediately compares it. | Agreement is only needed for proof/debug. Storing it keeps route-index status alive in prepared data even though the executable publication already exists. |
| Route 5 current-block join-source agreement for one prepared parallel-copy source fact. | Prepared edge publication fact, successor BIR block, predecessor/successor labels, source/destination prepared names, source/destination homes, and optional immediate source. | Existing local pattern in `attach_route5_current_block_join_source_if_agrees` queries a provided `Route5EdgeJoinSourceIndex` and checks uniqueness. | The source of truth for codegen is the prepared edge publication plus freshness authority. Route 5 agreement should be regenerated by diagnostic code that still wants it. |
| Route 7 comparison agreement for AArch64 fused compare/materialized condition lowering. | BIR block, condition or operand value, instruction index, and operand role. | `src/backend/mir/aarch64/codegen/comparison.cpp` already builds `route7_build_comparison_condition_index` around the relevant block. | It is target-specific validation, not a prealloc handoff fact. |
| Route 4 current-block call-boundary or indirect-callee producer agreement. | Current BIR block, BIR value, before-instruction index, and prepared producer kind. | AArch64 calls locally build a one-block Route 4 publication index before comparing current-block publication references. | This is a compatibility proof for AArch64 call lowering and should not become global prepared state. |
| Route 3/5 memory-source agreement for prepared edge publication diagnostics. | Prepared memory access lookup, prepared edge publication source fields, and the relevant BIR memory/source route when diagnostics are requested. | RV64 prepared edge publication emission currently attaches Route 5 and Route 3 agreement diagnostics near the emitter. | Memory access identity is required; numbered-route agreement status is only diagnostic proof. |

## Required Boundary

`prepare::PreparedFunctionLookups` should contain prepared-owned facts that
codegen can consume without reconstructing numbered BIR route state:

- call plans
- address materializations
- memory accesses
- move bundles
- value homes
- edge publications
- edge publication source producers
- branch stack-load authorities

`mir::prepared::PreparedMirCoreView` should expose those lookups together with
prepared control-flow, value-location, addressing, and BIR binding facts. It
should not grow Route 4, Route 5, Route 7, or route-index status fields as
stable state. If a dump or target assertion still needs historical agreement,
the agreement should be recomputed from local BIR and prepared facts at that
diagnostic surface.

## Retirement Consequences

- Route-index state is not a minimum BIR-to-prealloc input. The minimum input is
  the prepared semantic data already gathered in `PreparedFunctionLookups` plus
  BIR function/block/value references needed to tie prepared facts back to the
  source program.
- Route 4 block-entry attribution can be removed from prepared data once dump
  proof rows compare prepared facts directly or recompute Route 4 locally.
- Route 5 join-source agreement can move out of `PreparedCurrentBlockJoinParallelCopySourceFact`
  once AArch64 dispatch and prepared printer consumers stop depending on the
  stored `route5_*` annotation.
- Route 7 belongs to target comparison validation, not the BIR-to-prealloc
  handoff. It should be retired through an AArch64 comparison-proof replacement,
  not by adding Route 7 facts to prealloc state.
