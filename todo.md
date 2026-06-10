Status: Active
Source Idea Path: ideas/open/164_bir_call_use_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Call-Use Source Surfaces

# Current Packet

## Just Finished

Completed Step 1 for
`ideas/open/164_bir_call_use_source_annotation_schema.md`.
Inventoried the Route 6 call-use source surfaces and concrete Step 2 targets.

Prepared call-use oracle surfaces:

- Argument source materialization:
  `prepare::find_prepared_call_argument_source_producer_materialization(...)`,
  `PreparedCallArgumentSourceProducerMaterialization`,
  `PreparedCallArgumentSourceProducerMaterializationFor<T>`, and
  `prepared_call_argument_binary_producer_opcode_is_materializable(...)`.
  These expose same-block call argument source producers, producer instruction
  index, producer kind, produced value, and scalar materialization eligibility.
- Argument direct-global dependency:
  `PreparedDirectGlobalSelectChainDependency`,
  `prepared_direct_global_select_chain_dependency_available(...)`,
  `PreparedCallArgumentDirectGlobalSelectChainDependency`, and
  `find_prepared_call_argument_direct_global_select_chain_dependency(...)`.
  These expose source value name, direct global load presence, select-root
  status, and root instruction index.
- Argument publication/source routing:
  `PreparedCallArgumentPublicationSourceRouting` and
  `find_prepared_call_argument_publication_source_routing(...)`, with semantic
  source ids/base ids/base names/pointer deltas plus optional source selection
  and direct-global dependency pointers.
- Result provenance oracles:
  `PreparedCallResultPlan`, `PreparedCallResultLatePublicationFact`, and
  `find_prepared_call_result_late_publication(...)` for destination value
  availability only; `PreparedAfterCallResultLaneBinding` in the prepared
  after-call lane path for primary/result-lane identity.
- ABI-bound exclusion surfaces:
  `PreparedCallArgumentSourceSelection`,
  `PreparedCallArgumentSourceSelectionKind`,
  `PreparedMissingFrameSlotCallArgumentPublicationNeed`,
  `find_prepared_missing_frame_slot_call_argument_publication_need(...)`,
  `PreparedAggregateTransportPlan`, `PreparedAggregateTransportLane`,
  `PreparedAggregateTransportScratchRequirement`, `PreparedCallPlan`,
  `PreparedCallResultPlan`, `PreparedCallBoundaryEffectPlan`,
  `PreparedCallBoundaryMoveClassification`, and call-plan lookup helpers such
  as `find_indexed_prepared_call_plan(...)`,
  `find_indexed_prepared_immediate_call_argument(...)`, and
  `find_indexed_prepared_outgoing_stack_argument_area(...)`. These remain
  prepared/codegen-owned when they require ABI placement, outgoing stack,
  byval lanes, scratch, preservation, destination homes, or storage policy.

Existing BIR/MIR call-use surfaces:

- BIR `CallInst` already carries call boundary source payloads:
  `args`, `arg_types`, `arg_sources`, `result`, and `result_lanes`.
- BIR call argument payload types to normalize into Route 6 typed records:
  `CallArgumentSourceRelationship`,
  `CallArgumentSourceEncodingKind`,
  `CallArgumentSourceSelection`,
  `CallArgumentSourceSelectionKind`,
  `CallArgumentDirectGlobalSelectChainDependency`,
  `CallArgumentSourceProducerKind`,
  `CallArgumentSourceProducerMaterialization`, and
  `CallArgumentPublicationSourceRouting`.
- BIR result payload helpers:
  `CallResultSourceIdentity` and `CallResultLaneSourceIdentity`.
- BIR helper APIs:
  `find_call_argument_source_relationship(...)`,
  `find_call_argument_publication_source_routing(...)`,
  `find_call_argument_source_producer_materialization(...)`,
  `find_call_result_source_identity(...)`, and
  `find_call_result_lane_source_identity(...)`.
- Existing MIR query surfaces are adjacent rather than call-specific:
  `mir::find_bir_select_chain_direct_global_dependency(...)`,
  `mir::find_bir_select_chain_identity(...)`,
  `mir::find_bir_memory_access_identity(...)`,
  `mir::find_bir_same_block_load_local_source_identity(...)`,
  `mir::find_bir_current_block_publication_identity(...)`,
  `mir::find_bir_cfg_edge_publication_source_identity(...)`, and related
  Route 2/3/4/5 helpers. No dedicated public MIR call-use query exists yet.

Concrete Route 6 Step 2 BIR schema targets:

- Add typed Route 6 instruction-scoped records on/over `CallInst`, not whole
  prepared call plans:
  `Route6CallArgumentSourceRecord`,
  `Route6CallArgumentSourceProducerRecord`,
  `Route6CallArgumentDirectGlobalDependencyRecord`,
  `Route6CallArgumentPublicationSourceRecord`,
  `Route6CallResultSourceRecord`, and
  `Route6CallResultLaneSourceRecord`.
- Reuse Route 1/2/3/4/5 semantic payloads where possible:
  Route 1 source value/producer/materialization for same-block producers;
  Route 2 direct-global dependency identity for select-chain/global facts;
  Route 3 memory access identity for eligible load-local/global memory source
  facts; Route 4/5 publication source identity only as semantic value/source
  references, not publication execution policy.
- Tentative Route 6 fields:
  call instruction pointer/index, callee name/link id only as call identity,
  argument index, result lane index, source value/name/type/id, base
  value/name/id, pointer byte delta when semantic, source producer kind,
  producer instruction/index, produced value identity, materialization
  eligibility, direct-global dependency payload, optional memory/publication
  source reference, and explicit status.
- Tentative value annotation fields:
  result value/name/type/id, call instruction/index, result role
  primary/lane, lane index, and aliases-primary-result.
- Tentative statuses:
  `Unavailable`, `Available`, `MissingCall`, `WrongCall`,
  `MissingArgument`, `MissingResult`, `MissingSourceRelationship`,
  `MissingSourceValue`, `MissingSourceProducer`, `MissingDirectGlobal`,
  `MissingMemorySource`, `MissingPublicationSource`, `AbiBoundExcluded`,
  `DuplicateRelationship`, `DuplicateResultLane`, and `NoMatch`.
- Tentative source kinds/value roles:
  source kinds `Unknown`, `Immediate`, `ArgumentValue`, `BaseValue`,
  `LoadLocal`, `LoadGlobal`, `Binary`, `DirectGlobalSelectChain`,
  `MemorySource`, `PublicationSource`, and `AbiBoundExcluded`; value roles
  `ArgumentSource`, `ArgumentBase`, `Result`, `ResultLane`, and
  `PublicationSource`.
- Optional Step 3 index shape:
  `Route6CallUseSourceIndex` keyed by function/block label id,
  call instruction index, callee link/name, argument index, result value
  name/type, and result lane value name/type. The index should store pointers
  to Route 6 records rebuilt from BIR `CallInst` and value payloads, not clone
  prepared call plans.

Step 2 oracle coverage to add in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:

- Positive argument producer cases: load-local source, materializable binary,
  and nonmaterializable binary preserving eligibility.
- Positive direct-global argument case: select-root/direct global dependency
  compared with prepared Route 2 dependency and existing MIR Route 2 query.
- Positive semantic publication/memory source cases: computed/base value,
  frame-slot value as semantic slot/value identity, load-local memory source
  when expressible through Route 3, and current/edge publication source when
  expressible through Route 4/5 semantic records.
- Positive result cases: primary result value identity and result lane identity
  against prepared result/lane oracles.
- Negative cases: wrong call instruction index/detached call, out-of-range
  argument, duplicate argument source relationship, missing source producer,
  producer after call boundary, explicit unavailable routing, missing result,
  duplicate lane, direct-global absent, memory/publication source absent,
  no-match value/type, and ABI-bound exclusion for source selections that
  require register names, stack offsets, byval lane layout, outgoing stack
  area, preservation/clobber, scratch, aggregate transport, destination homes,
  helper/carrier protocol, or register/storage policy.

## Suggested Next

Execute Step 2 by adding typed Route 6 BIR call-use annotation records and
construction helpers only, with focused oracle coverage in
`backend_prepared_lookup_helper_test.cpp`.

## Watchouts

- Keep Route 6 annotations target-neutral and semantic.
- Do not import ABI register names, stack slots, outgoing stack area sizing,
  variadic FPR counts, clobber/preservation sets, byval lane layout, scratch
  resources, helper/carrier protocol, destination homes, aggregate transport
  layout, or ABI/layout-bound publication-routing source-selection reads.
- Do not copy `PreparedCallArgumentPlan`, call result plans, outgoing-stack
  plans, or aggregate transport lane records as the BIR schema shape.
- Preserve explicit result, direct-global, memory/publication-source,
  wrong-call, missing-source, ABI-bound exclusion, and no-match cases where
  applicable.
- Existing `CallArgumentSourceSelection` still contains quarantine fields for
  prepared-only layout comparisons. Step 2 should keep new Route 6 records
  separate from those ABI/layout fields and may use them only for negative
  ABI-bound exclusion assertions.
- The first code-changing proof should be narrow:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log`.

## Proof

Inventory-only Step 1. No build or test proof required.

Additional local validation: `git diff --check` passed.
