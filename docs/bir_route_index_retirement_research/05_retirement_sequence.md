# Route Index Retirement Sequence

This document defines a behavior-preserving retirement sequence for the
numbered BIR route APIs. The sequence intentionally introduces each named view
before moving consumers, keeps compatibility adapters in place while dumps and
target validation still mention route numbers, and defers high-risk routes
until later BIR/prealloc rebuild work owns the missing prepared facts.

The durable direction is to expose named view boundaries such as
`BirPublicationView`, `BirControlValueView`, `BirProducerView`,
`BirMemoryAccessView`, `BirCallBoundaryView`, and `BirReturnChainView` while
keeping old `Route1` through `Route8` builders available as private
compatibility implementation details during migration.

## Sequencing Principles

- Introduce a named view or named proof adapter before moving any consumer.
- Preserve runtime behavior and dump content in each phase unless that phase
  explicitly owns the dump-policy rewrite.
- Keep `PreparedFunctionLookups`, `PreparedEdgePublicationLookups`,
  `PreparedMoveBundleLookups`, prepared value homes, freshness records, and
  `PreparedMirCoreView` as the prepared authority boundary.
- Treat Route 4, Route 5, and Route 7 status fields as compatibility proof
  data, not publication, freshness, stack destination, or move authority.
- Do not delete a route-numbered API until all consumers have moved to a named
  view or a named compatibility proof adapter and the matching proof command is
  green.

## Phase Plan

| Phase | Commit intent | Main surfaces | Proof command | Rollback point |
| --- | --- | --- | --- | --- |
| 0 | Add named compatibility view wrappers that forward to existing route builders without moving consumers. | `src/backend/bir/bir.hpp`, `src/backend/bir/bir_route_index.hpp`, `src/backend/bir/bir_route_facade.cpp`; new names such as `BirPublicationView` and `BirCompatibilityProofView`. | `cmake --build build --target c4c_backend_tests && ctest --test-dir build -R 'backend|prealloc|mir' --output-on-failure` | Revert the wrapper commit; no consumer should depend on the new names yet. |
| 1 | Move the first low-risk consumer: Route 4 block-entry attribution in prepared lookup proof code. | `attribute_route4_block_entry_publication_if_agreeing` and `find_prepared_current_block_entry_publication` in `src/backend/prealloc/prepared_lookups.cpp`; `PreparedCurrentBlockEntryPublication` fields in `src/backend/prealloc/value_locations.hpp`; Route 4 validation through `route_index_validate_block_entry_publication_reference`. | `ctest --test-dir build -R 'prealloc|prepared|backend' --output-on-failure` plus a focused dump comparison for a block-entry publication case if the touched tests expose one. | Revert the consumer migration while keeping Phase 0 wrappers; old `route_index_reference_facade` and Route 4 fields remain available. |
| 2 | Rename the Route 4 proof vocabulary at the prepared-printer edge without changing printed facts yet. | `find_agreeing_route4_block_entry_publication` and block-entry proof rows in `src/backend/prealloc/prepared_printer/value_locations.cpp`; `PreparedCurrentBlockEntryPublicationQueryInputs`. | `ctest --test-dir build -R 'prealloc|prepared_printer|backend' --output-on-failure` | Revert the printer-only commit; prepared lookup behavior remains unchanged. |
| 3 | Move Route 7 facade consumers to a named comparison proof adapter while preserving AArch64 comparison behavior. | `src/backend/mir/aarch64/codegen/comparison.cpp`; `route7_build_comparison_condition_index`; `route_index_validate_materialized_condition_reference`; `route_index_validate_comparison_operand_reference`; `Route7IndexReferenceValidation`. | `ctest --test-dir build -R 'aarch64|comparison|backend' --output-on-failure` | Revert the AArch64 comparison commit; Route 7 facade validation remains intact. |
| 4 | Move Route 5 stored join-source agreement behind a named publication proof adapter, but keep executable prepared edge publication unchanged. | `attach_route5_current_block_join_source_if_agrees`, `route5_join_source_record_agrees_with_prepared_fact`, and `prepare_current_block_join_parallel_copy_source_facts` in `src/backend/prealloc/publication_plans.cpp`; `PreparedCurrentBlockJoinParallelCopySourceFact` in `src/backend/prealloc/publication_plans.hpp`; AArch64 `dispatch_producers.cpp`; prepared printer `select_chains.cpp`. | `ctest --test-dir build -R 'prealloc|publication|dispatch|riscv|aarch64' --output-on-failure` | Revert the Route 5 adapter and consumer commit; keep `route5_join_source`, `route5_join_source_status`, and `route5_join_source_agrees` fields until all consumers move. |
| 5 | Delete or privatize the narrow `bir_route_index` facade only after Route 4 and Route 7 consumers no longer need public route-index status. | `RouteIndexReferenceFacade`, `RouteIndexRoute`, `RouteIndexRecordReference`, `Route4IndexReferenceValidation`, `Route7IndexReferenceValidation`, and `src/backend/bir/bir_route_facade.cpp`. | `cmake --build build --target c4c_backend_tests && ctest --test-dir build -R 'backend|prealloc|aarch64|prepared' --output-on-failure` | Restore the facade header/implementation commit; named views can continue to coexist with the facade. |
| 6 | Retire route-numbered dump spelling only under the later dump-policy packet, not as part of the first consumer moves. | `src/backend/prealloc/prepared_printer/value_locations.cpp`, `src/backend/prealloc/prepared_printer/select_chains.cpp`, and any route/prepared expectation files selected by the test-policy plan. | The command selected by the dump-policy packet, likely a focused dump-test subset plus `ctest --test-dir build -R 'prepared|dump|backend' --output-on-failure`. | Revert the dump spelling commit; named implementation can remain if the printed compatibility labels need to come back. |

## First Low-Risk Consumer Migration

The first low-risk consumer migration should be Route 4 block-entry
attribution in `src/backend/prealloc/prepared_lookups.cpp`.

Reasons:

- It is already localized in
  `attribute_route4_block_entry_publication_if_agreeing`.
- It builds a temporary Route 4 publication index and immediately validates one
  prepared block-entry publication with
  `route_index_validate_block_entry_publication_reference`.
- Its stored `PreparedCurrentBlockEntryPublication` route fields in
  `src/backend/prealloc/value_locations.hpp` are proof attribution, not
  executable destination authority.
- The printer consumer,
  `find_agreeing_route4_block_entry_publication` in
  `src/backend/prealloc/prepared_printer/value_locations.cpp`, reads the same
  attribution shape and can be migrated after the builder.

The first implementation commit should therefore add a named publication-proof
adapter that forwards to the existing Route 4 logic, then change only the
prepared lookup attribution call site to request a named
block-entry-publication agreement result. It should not remove the
`route4_block_entry_publication_*` compatibility fields in the same commit.
Those fields remain the rollback and dump-compatibility surface until the
printer and test policy are ready.

## Required Compatibility Layer

The compatibility layer should be explicit rather than accidental. During the
migration, the old route builders can remain private implementation details
behind named views:

- `Route4PublicationAvailabilityIndex` can back
  `BirPublicationView::BlockEntry` and a named publication agreement proof.
- `Route5EdgeJoinSourceIndex` can back
  `BirPublicationView::CfgEdge` and a named current-block join-source
  agreement proof.
- `Route7ComparisonConditionIndex` can back
  `BirControlValueView::Comparison` and a named comparison agreement proof.
- `Route1ProducerIndex`, `Route2SelectChainValueIndex`,
  `Route3MemoryAccessIndex`, `Route6CallUseSourceIndex`, and
  `Route8ReturnChainIndex` can stay route-numbered internally until their
  consumers have named producer, control, memory, call, and return views.

The named view must preserve the same records, statuses, and pointer lifetimes
as the route-numbered implementation during the compatibility phase. A commit
that changes authority or storage semantics is not part of route retirement; it
belongs to a later capability rebuild.

## Rollback Rules

Each phase has a small rollback point:

1. Named wrappers only: revert the wrapper commit because no consumer moved.
2. First Route 4 consumer move: revert the prepared lookup call-site commit and
   keep the wrappers.
3. Route 4 printer move: revert the printer commit; prepared lookup
   attribution and compatibility fields remain available.
4. Route 7 comparison move: revert the AArch64 comparison adapter commit; the
   old `route_index_reference_facade` validation is still available.
5. Route 5 join-source move: revert the adapter and consumer commit; keep the
   stored `route5_*` fields until every printer and target consumer is moved.
6. Facade deletion: restore `bir_route_index.hpp` and `bir_route_facade.cpp`
   from the previous commit if any external or test consumer still requires
   route-index status.

Rollback should never require expectation downgrades, unsupported markers, or
runtime behavior changes. If rollback would require changing expected behavior,
the phase was too large.

## Focused Proof Commands

These are proof commands for implementation packets that follow this research:

- Wrapper-only named view introduction:
  `cmake --build build --target c4c_backend_tests && ctest --test-dir build -R 'backend|bir' --output-on-failure`
- Route 4 prepared lookup migration:
  `ctest --test-dir build -R 'prealloc|prepared|backend' --output-on-failure`
- Route 4 prepared-printer compatibility migration:
  `ctest --test-dir build -R 'prepared_printer|prealloc|backend' --output-on-failure`
- Route 7 AArch64 comparison proof migration:
  `ctest --test-dir build -R 'aarch64|comparison|backend' --output-on-failure`
- Route 5 publication/join-source proof migration:
  `ctest --test-dir build -R 'publication|dispatch|prealloc|aarch64|riscv' --output-on-failure`
- Facade privatization or deletion:
  `cmake --build build --target c4c_backend_tests && ctest --test-dir build -R 'backend|prealloc|aarch64|prepared' --output-on-failure`

The exact subset can be narrowed by the supervisor for each implementation
packet, but each packet needs at least one fresh compile or CTest proof unless
it is documentation-only.

## Deferred Routes

Several routes must remain untouched until later BIR/prealloc rebuild work
owns the missing semantic replacement. Do not fold these into the first Route 4
or facade-retirement commits.

| Route | Defer until | Why |
| --- | --- | --- |
| Route 1 | A named producer view can replace direct target-lowering producer queries. | Route 1 is the shared producer substrate for Route 2, Route 4, Route 5, Route 6, Route 7, AArch64 dispatch, AArch64 calls, and prepared materialization checks. Moving it early risks behavior changes across most lowering paths. |
| Route 2 | A named control/select-chain view is available and prepared value materialization no longer depends on route-numbered select-chain records. | Select-chain records feed call dependencies and AArch64 value materialization; the route number is not only facade residue. |
| Route 3 | A named memory access view is available and memory-source proof is separated from prepared memory authority. | Route 3 participates in memory access identity, Route 5 source proof, Route 6 fallback, AArch64 globals/calls, and RV64 edge-publication diagnostics. |
| Route 5 | Prepared edge publication, source freshness, move bundle authority, and printer policy have a named publication proof path. | Route 5 includes real BIR edge publication semantics plus stored `route5_join_source_*` proof residue. It is too central to move before the prepared publication and dump surfaces are split. |
| Route 6 | A named call boundary view exists and target call lowering consumes prepared call plans for authority. | Route 6 call argument/result source reconstruction is still used by AArch64 calls and call diagnostics. |
| Route 7 | A named comparison/control-value proof adapter exists and AArch64 comparison lowering is proven against it. | Route 7 must stay confined to comparison/control-value validation and must not be used for publication, freshness, or destination authority. |
| Route 8 | Return-chain ownership is decided separately from value-home or stack destination authority. | Route 8 is target-lowering helper state for return chains, not part of `bir_route_index`; moving it is not required for the first facade retirement. |

## Acceptance Criteria For Retirement

A route-numbered public surface can be retired only when all of these are true:

- A named view or named proof adapter exists and preserves behavior.
- All direct consumers have moved to the named surface.
- Compatibility dump spelling is either preserved intentionally or rewritten by
  the test/dump policy packet.
- Focused proof commands are green for the moved consumer family.
- No route-numbered proof field is used as prepared publication, freshness,
  stack destination, move-bundle, or MIR authority.

If any of those conditions is false, keep the route-numbered API as a
compatibility adapter and defer deletion.
