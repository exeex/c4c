# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect the prepared call-boundary oracle

## Just Finished

Step 1 inspected the prepared call-boundary oracle and classified the
BIR-owned semantic facts versus ABI/prealloc/codegen-owned fields before any
implementation.

BIR may own these call argument source facts:

- `PreparedCallArgumentPlan::{source_encoding,source_value_id,source_base_value_id,source_base_value_name,source_pointer_byte_delta}` when used only as source identity for a call-use relationship. `source_literal`, `source_symbol_name`, and `source_symbol_name_id` are semantic callee/argument source payloads, but later BIR modeling should keep the existing LinkNameId authority and avoid raw spelling fallbacks.
- `PreparedCallArgumentSourceSelection::kind` for the semantic source shape:
  `FrameSlotAddress`, `FrameSlotValue`, `LocalFrameAddressMaterialization`,
  `PriorPreservation`, and `ByvalRegisterLane`. BIR may keep the selected
  source value/name, base value, target-neutral pointer delta, selected source
  slot identity, and the source memory extent only as source/access identity.
- `PreparedCallArgumentSourceSelection::{
  source_value_id,source_value_name,source_base_value_id,
  source_pointer_byte_delta,address_materialization_block_label,
  address_materialization_inst_index,address_materialization_frame_slot_id,
  address_materialization_byte_offset}` as relationship identity from the call
  use back to the producing BIR value or frame-address materialization. The
  block/inst coordinates must identify the BIR producer, not an emission site.
- `PreparedCallArgumentSourceProducerMaterialization::{producer,materializable}`
  where `producer` is the same-block BIR producer and `materializable` is the
  current semantic eligibility for call-argument source reuse. Existing
  materializable producers are load-local and selected binary opcodes
  (`Add/Sub/And/Or/Xor/Mul/SDiv/SRem`); nonmaterializable opcodes should stay
  negative coverage.
- `PreparedCallArgumentDirectGlobalSelectChainDependency::{
  available,source_value_name,direct_global_dependency.contains_direct_global_load,
  direct_global_dependency.root_is_select,
  direct_global_dependency.root_instruction_index}` as the direct-global
  select-chain dependency identity for the call argument.
- `PreparedCallArgumentPublicationSourceRouting::{available,source_encoding,
  source_value_id,source_base_value_id,source_base_value_name,
  source_pointer_byte_delta,source_selection,
  direct_global_select_chain_dependency}` as the prepared oracle query shape
  for publication-source routing, limited to the semantic links above.
- Memory/access source links that are already target-neutral BIR identities,
  such as same-block load-local/global access and frame-slot source identity,
  may be referenced by the call-use relationship. Do not import final AArch64
  addressing-form decisions.

BIR may own only these call result source/publication facts:

- The call result value identity attached to the BIR `CallInst` result and
  result lanes: `PreparedCallResultPlan::destination_value_id` and
  `PreparedAfterCallResultLaneBinding::{value_id,value_name,block_index,
  instruction_index}` when they mean "this BIR call position produced this
  semantic result value." `lane_index` may be useful only as a BIR result-lane
  ordinal if the BIR call already exposes lanes; it must not imply ABI lane
  placement.
- `PreparedCallResultLatePublicationFact::available` can be compared as an
  oracle for whether late publication exists, but the fields that make it
  available are placement facts. Treat the result source as the call result
  value/lane itself, not the ABI register or stack carrier.

These prepared fields must remain outside BIR:

- Argument/result ABI placement: destination/source register names, register
  banks when used for ABI class/placement, contiguous widths, occupied register
  names, register placements, stack offsets, stack sizes, outgoing stack area,
  variadic FPR argument count, and ABI binding records.
- Aggregate/byval transport policy: `PreparedAggregateTransportPlan`, chunks,
  lanes, destination lane registers, destination stack offsets/sizes, scratch
  requirements, payload copy sizing as an ABI transport decision, and byval
  lane materialization protocols.
- Preservation/clobber policy: preservation route storage details, preserved
  register/stack placements, callee-saved save indexes, spill-slot placement,
  clobbered registers, preservation source/destination endpoints, and call
  boundary effect plans. BIR may refer to prior preserved source identity only
  as a semantic origin after equivalence is proven.
- Result late-publication placement: source/destination storage kinds,
  source/destination register names/banks, destination slot/stack offset,
  source-register publication availability, alias availability, FPR/Vreg
  retarget availability, after-call ABI bindings, and final move records.
- Helper/carrier/final-lowering policy: indirect-call scratch/carriers,
  memory-return ABI slots, runtime helper protocols, AArch64 instruction
  selection, materialization ordering, move scheduling, and diagnostics that
  enforce missing prepared authority.

Existing coverage found:

- `backend_prepared_lookup_helper_test.cpp` covers prepared/BIR equivalence for
  same-block scalar producers, current-block publication identity, same-block
  global-load access, same-block load-local source identity, integer constants,
  select-chain source/direct-global/materialization identity, and CFG
  edge-publication source identity. It also directly covers
  `find_prepared_call_argument_direct_global_select_chain_dependency` positive
  and fail-closed behavior.
- `backend_prepare_frame_stack_call_contract_test.cpp` covers prepared call
  argument source shapes for symbol/computed source summaries, stack/frame-slot
  source selection, local frame address materialization, derived pointer delta,
  prior-preservation selection, byval register-lane aggregate transport,
  direct-global select-chain call-argument routing, missing frame-slot
  publication need, and AArch64 consumers rejecting missing source-selection
  authority.
- `backend_prealloc_call_boundary_classification_test.cpp` covers call
  argument/result boundary move classification, missing ABI index/binding,
  missing/mismatched result plan, and unsupported move op kinds.
- `backend_prepared_printer_test.cpp` covers dump visibility for call argument
  source shape, direct-global select-chain fields, prior-preservation source
  selection, stack-slot prior preservation, and structured call result
  placement. This is visibility/fixture coverage, not BIR equivalence.

Missing narrow coverage before implementation:

- A call-boundary-specific BIR/prepared equivalence test for scalar call
  argument `source_encoding/source_value_id` routing against
  `find_prepared_call_argument_publication_source_routing`.
- Positive and negative call-argument equivalence tests for
  `PreparedCallArgumentSourceProducerMaterialization`, including load-local,
  materializable binary opcode, nonmaterializable binary opcode, missing
  producer, and producer after the call instruction.
- Call-argument equivalence coverage for direct-global select-chain routing
  through `PreparedCallArgumentPublicationSourceRouting`, not only the generic
  select-chain helper.
- Call-argument equivalence coverage for frame-slot value, frame-slot address,
  local-frame-address materialization, derived pointer delta, prior
  preservation, byval register-lane source identity, and unavailable
  source-selection paths, with assertions that no ABI register/stack placement
  fields are copied into the BIR answer.
- Result-side narrow coverage that compares BIR call result value/lane identity
  against `PreparedCallResultPlan::destination_value_id` and
  `PreparedAfterCallResultLaneBinding` while explicitly excluding result
  register names, destination homes, ABI bindings, late-publication alias
  booleans, and move records.

## Suggested Next

Delegate Step 2 to add a narrow BIR call-use source relationship/query surface
for call arguments only, starting with scalar source identity and
publication-source routing fields; do not switch AArch64/prealloc consumers yet.

## Watchouts

- Keep ABI register/stack placement, aggregate transport lanes, scratch,
  variadic state, preservation/clobber policy, destination homes, helper
  protocols, and final call lowering outside BIR.
- Treat prepared call plans and call-source helper queries as the oracle until
  BIR equivalence is proven one fact family at a time.
- Do not weaken tests, rewrite expectations, or add named-case shortcuts to
  claim progress.
- Result facts are not symmetrical with argument facts: current prepared result
  helpers mostly expose ABI placement and move/lane bindings. Only call result
  value/lane identity is a BIR candidate for this idea.
- Do not import `PreparedCallArgumentPlan`, `PreparedCallResultPlan`,
  `PreparedAggregateTransportPlan`, `PreparedCallBoundaryEffectPlan`, or
  `PreparedAbiBinding` wholesale into BIR.

## Proof

No build required or run for this inspection-only packet. No `test_after.log`
was produced because the delegated proof explicitly required no build.
