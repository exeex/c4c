Status: Active
Source Idea Path: ideas/open/01_shared_prepared_call_argument_source_selection.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Source-Choice Contract Mapping

# Current Packet

## Just Finished

Step 1 of `plan.md` completed the source-choice contract mapping for selected
`BeforeCall` call-argument moves.

Contract name: `PreparedCallArgumentSourceSelection`.

Attachment point: add one optional shared source-selection fact to each selected
`PreparedCallArgumentPlan`, keyed by the existing call-plan argument identity:
`PreparedCallPlan::{block_index,instruction_index}` plus
`PreparedCallArgumentPlan::arg_index`. Target emitters should reach it through
the already-classified `PreparedCallArgumentPlan` returned by
`classify_prepared_call_boundary_move`, not by rescanning BIR or redoing local
source-choice tests.

Required source-selection fields:

- `kind`: target-independent enum covering `None`, `PriorPreservation`,
  `LocalFrameAddressMaterialization`, `FrameSlotAddress`, `FrameSlotValue`,
  and `ByvalRegisterLane`.
- `source_value_id` and `source_value_name`: semantic source identity for the
  selected argument source. Prefer the existing argument `source_value_id`; use
  the move `from_value_id` only as the same fallback AArch64 currently uses.
- `source_home_kind`: copied from the selected `PreparedValueHome` when the
  source depends on a value home (`Register`, `StackSlot`, or
  `PointerBasePlusOffset`).
- `source_slot_id`, `source_stack_offset_bytes`, `source_size_bytes`, and
  `source_align_bytes`: required for `FrameSlotValue`, `FrameSlotAddress`, and
  stack-backed byval lane sources. These come from the prepared value home,
  prepared memory access, prepared frame slot, or preserved stack slot that
  already proves the address.
- `source_base_value_id` and `source_pointer_byte_delta`: required for
  aggregate-address stack publication when a pointer-base-plus-offset home is
  the selected source.
- `address_materialization_block_label`, `address_materialization_inst_index`,
  `address_materialization_frame_slot_id`, and
  `address_materialization_byte_offset`: required when the selected source is a
  local frame/address materialization. These mirror the matching
  `PreparedAddressMaterialization` fact and must not encode AArch64 emission
  details.
- `preserved_call_block_index`, `preserved_call_instruction_index`,
  `preservation_route`, and preserved stack slot fields: required for
  `PriorPreservation`. The chosen preserved value must be the latest indexed
  prior preserved value for the source id before the current call.
- `byval_lane_extent_bytes` and `byval_lane_source_instruction_index`: required
  for `ByvalRegisterLane`, with extent limited to the existing prepared lane
  rule (`0 < extent <= 16`) and sourced from prepared lane stores or the
  selected source home size, not from target-local pattern matching.
- Destination ABI fields are not part of the source-selection fact. Emitters
  continue to use `PreparedAbiBinding`, `PreparedMoveResolution`, and existing
  argument destination fields for register or stack destinations.

Mapped AArch64 local choices:

- Prior preservation: `lower_before_call_move` currently calls
  `find_prior_preserved_value_for_call_argument`, then
  `make_prior_preserved_call_argument_source`, when the selected register
  argument is not one of the frame-slot address, local-frame address, f128, or
  byval source cases. The shared fact should record
  `PriorPreservation` from `PreparedCallPlanLookups::prior_preserved_by_value`
  and the selected `PreparedCallPreservedValue`.
- Local-frame address materialization: AArch64 currently tests
  `make_local_frame_address_call_argument_source` for register-source aggregate
  address publication. The shared fact should record
  `LocalFrameAddressMaterialization` from a matching
  `PreparedAddressMaterializationKind::FrameSlot` with
  `allows_local_aggregate_address_publication`, matching value name, block, and
  instruction dominance at or before the call move.
- Frame-slot address source: AArch64 currently chooses an address source via
  `make_sret_memory_return_address_source` or
  `make_frame_slot_call_argument_address_source` for GPR destinations from
  frame-slot encoded arguments. The shared fact should record
  `FrameSlotAddress` with the selected frame slot and byte offset.
- Frame-slot value source: AArch64 currently chooses a load/copy source via
  `make_frame_slot_call_argument_source` for scalar GPR/FPR, binary128, normal
  stack argument, and aggregate stack-copy cases. The shared fact should record
  `FrameSlotValue` with the selected prepared memory access or value-home
  fallback fields.
- Byval register-lane source: AArch64 currently recognizes
  `move.reason == "call_arg_byval_aggregate_register_lanes"` and calls
  `prepared_byval_lane_extent_bytes` plus
  `make_byval_register_lane_prepared_source`, with frame-slot fallback for
  stack-backed lanes. The shared fact should record `ByvalRegisterLane` with
  lane extent, source instruction index, and the selected prepared memory
  source fields.

## Suggested Next

Execute Step 2 of `plan.md`: add the target-independent
`PreparedCallArgumentSourceSelection` representation to shared prepared call
plans, populate it for the five mapped choices above, and expose it from the
classified `PreparedCallArgumentPlan` surface without changing AArch64
emission behavior yet.

## Watchouts

- Do not consolidate or delete AArch64 `calls*.cpp` files in this prerequisite
  runbook.
- Do not encode AArch64-specific machine-node emission details in the shared
  prepared fact.
- Do not claim progress through testcase-shaped matching, expectation
  weakening, or wrapper-only renames of the existing local decision tree.
- The first implementation slice should not move destination-register or
  destination-stack selection into the new fact. The new fact owns source
  choice only.
- `ByvalRegisterLane` is allowed to carry the existing target-neutral move
  reason as source classification input, but it must publish the resulting
  selected source and extent as shared prepared data so AArch64 no longer owns
  that source-choice decision after Step 3.

## Proof

Mapping-only packet. Used `c4c-clang-tool-ccdb`/`c4c-clang-tool` to inspect
`lower_before_call_move`, shared prepared call-plan structures, lookup helpers,
and source helper callees. No implementation files were modified, so no build,
ctest, or `test_after.log` was required.

First implementation proof target: extend prepared-plan contract coverage so a
shared prepared call plan visibly publishes the new source-selection fact for
prior preservation, local-frame address materialization, frame-slot address,
frame-slot value, and byval register-lane sources. The natural focused tests are
the existing prepared call/source-shape surfaces under
`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`, plus a
lookup/printer assertion if Step 2 adds a dedicated lookup or dump field.
