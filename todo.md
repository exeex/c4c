Status: Active
Source Idea Path: ideas/open/07_aarch64_call_boundary_move_emission_only.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Remaining Selection Decisions In Call Moves

# Current Packet

## Just Finished

Step 1 mapping completed for the AArch64 call-boundary move cluster.

Pure target-emission branches/helpers to keep in AArch64:

- `lower_before_call_move`: destination register construction from
  `PreparedAbiBinding`, `PreparedCallArgumentPlan`, and
  `PreparedMoveResolution` fields; final `CallBoundaryMoveInstructionRecord`
  population and `make_call_boundary_machine_instruction` emission.
- `lower_before_call_move`: selected register-to-register argument moves where
  prepared source/destination register facts already agree with
  `source_home`; includes GPR, scalar FPR, full-width F128, and F128 constant
  carrier emission.
- `lower_before_call_move`: scalar frame-slot-to-register and frame-slot-to-stack
  stores after a complete `FrameSlotValue` source has been selected.
- `lower_before_call_move`: stack destination formation through
  `make_stack_call_argument_destination` from prepared binding/move/argument
  stack offsets.
- `calls_argument_sources.cpp`: `make_selected_frame_slot_source`,
  `make_selected_local_frame_address_source`, and
  `make_prior_preserved_call_argument_source` when they only translate
  `PreparedCallArgumentSourceSelection` fields into `MemoryOperand` or
  `RegisterOperand`.
- `calls_argument_sources.cpp`: register-view, immediate, F128 carrier,
  indirect callee register, memory-return storage, and aggregate pointer
  operand construction from already selected prepared facts.
- `calls_dispatch_bridge.cpp`: `find_prepared_frame_slot_call_argument_move`
  and destination register conversion inside
  `materialize_missing_frame_slot_call_arguments`, once the source selection is
  supplied by prepared records.

Branches/helpers that still decide semantic source choice and should be
redirected to prepared facts:

- `lower_before_call_move` prefilter booleans
  `frame_slot_address_argument`, `local_frame_address_argument`,
  `register_byval_argument`, and `indirect_byval_argument` still call source
  discovery helpers to decide which special source wins. These should read
  `argument.source_selection.kind` instead:
  `FrameSlotAddress`, `LocalFrameAddressMaterialization`, or
  `ByvalRegisterLane`.
- `make_frame_slot_call_argument_source` still scans
  `PreparedAddressingFunction::accesses` by result name and falls back to
  `source_home`/`argument` offsets when `argument.source_selection` is absent.
  Required prepared fact: `PreparedCallArgumentSourceSelection{kind =
  FrameSlotValue, source_slot_id, source_stack_offset_bytes,
  source_size_bytes, source_align_bytes, source_value_id/source_value_name}`.
- `make_sret_memory_return_address_source` still derives a frame-slot address
  from `call_plan.memory_return` when no source selection exists. Required
  prepared fact: `PreparedCallArgumentSourceSelection{kind =
  FrameSlotAddress}` populated from `PreparedMemoryReturnPlan`.
- `make_frame_slot_call_argument_address_source` still scans
  `PreparedAddressingFunction::address_materializations` and then scans
  stack-layout objects by lane-shaped names when no source selection exists.
  Required prepared fact: `PreparedCallArgumentSourceSelection{kind =
  FrameSlotAddress, address_materialization_* or source_slot_id/offset/size}`.
- `make_local_frame_address_call_argument_source` still scans latest frame-slot
  address materializations and stack-layout objects by
  `local_frame_address_name_matches` when no source selection exists.
  Required prepared fact: `PreparedCallArgumentSourceSelection{kind =
  LocalFrameAddressMaterialization, address_materialization_*,
  source_size_bytes, source_align_bytes}`.
- `make_call_move_local_aggregate_frame_address_source` in `calls_moves.cpp`
  is the same local-aggregate frame-address recovery path for call moves.
  Required prepared fact: the same
  `LocalFrameAddressMaterialization` source selection, rather than target-local
  name matching.
- `selected_byval_lane_extent_bytes` still falls back to
  `prepared_byval_lane_extent_bytes`, which derives extent from the move,
  `source_home`, lane count, and payload stores when no selected byval source
  exists. Required prepared fact: `PreparedCallArgumentSourceSelection{kind =
  ByvalRegisterLane, byval_lane_extent_bytes,
  byval_lane_source_instruction_index}`.
- `lower_before_call_move` byval register-lane branches still fall back through
  `has_byval_register_lane_payload_stores`,
  `collect_byval_register_lane_stores`, `aggregate_lane_store_memory`, and
  `make_fragmented_*` helpers to reconstruct source bytes. Required prepared
  fact: byval lane source selection should include the exact byte source
  facts needed by `make_byval_register_lane_prepared_source`; otherwise this is
  a missing prepared fact blocker, not an AArch64 fallback.
- `lower_before_call_move` aggregate stack-copy branch still recovers
  `source_base_value_id` through `find_prepared_value_home` and uses
  `source_home->pointer_byte_delta`. Required prepared fact:
  `PreparedCallArgumentSourceSelection` or `PreparedCallArgumentPlan` must
  supply the selected aggregate address base value/register and byte delta.
- `lower_before_call_move` symbol-address materialization skip scans
  `PreparedAddressingFunction::address_materializations` at the call site.
  Required prepared fact: a source-selection/materialization lookup that marks
  the symbol address as already materialized for this call argument.
- `lower_value_moves` `BeforeInstruction` path still calls
  `find_prior_stack_preserved_value_before_instruction`, scanning previous call
  plans for stack preservation. Required prepared lookup:
  `PreparedCallPlanLookups::prior_preserved_by_value` or an indexed boundary
  effect lookup should supply the exact prior preserved source.
- `materialize_missing_frame_slot_call_arguments` still chooses between
  `make_missing_local_aggregate_frame_address_source` and
  `make_frame_slot_call_argument_source`. Required prepared fact:
  `argument.source_selection` should select
  `LocalFrameAddressMaterialization`, `FrameSlotAddress`, or `FrameSlotValue`
  before the bridge emits anything.

Prepared source authority already present:

- `PreparedCallArgumentPlan::source_selection` with kinds
  `PriorPreservation`, `LocalFrameAddressMaterialization`, `FrameSlotAddress`,
  `FrameSlotValue`, and `ByvalRegisterLane`.
- `PreparedCallArgumentSourceSelection` fields for source value/home identity,
  frame slot offset/size/align, address materialization identity,
  preservation route/register/stack facts, and byval lane extent.
- `select_prepared_call_argument_source` in `prealloc/call_plans.cpp` is the
  current planner that should own source-kind choice; missing data should be
  recorded as incomplete selection there or in prepared indexed lookups.

## Suggested Next

Start Step 2 with focused coverage for source kinds that already have complete
`PreparedCallArgumentSourceSelection` records: prior preservation,
frame-slot value, frame-slot address, local frame-address materialization, and
byval register lane where the selected source bytes are complete.

## Watchouts

- Keep source-selection authority in prepared call facts, not AArch64 emission.
- Do not touch the transient `review/` artifacts unless explicitly delegated.
- Treat expectation weakening or named-test shortcuts as route failures.
- Blockers for later implementation are missing prepared source facts, not
  opportunities to preserve target-local fallback scans.
- Byval lane payload-store discovery and local aggregate stack-object/name
  matching are the highest-risk remaining semantic reconstruction paths.
- `lower_value_moves` prior-stack preservation is outside direct call-argument
  emission but still uses call-plan history to choose a semantic source.

## Proof

No build proof required for this todo-only mapping packet. Used
`c4c-clang-tool-ccdb` symbol/signature/callee queries plus targeted source
inspection; no code or tests were changed, and `test_after.log` was not
updated.
