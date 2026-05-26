Status: Active
Source Idea Path: ideas/open/17_aarch64_absent_selection_fallback_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit the Two Fallback Families

# Current Packet

## Just Finished

Step 1: Audit the Two Fallback Families completed as an inspection-only
packet. Fallback map:

- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp:68`
  `materialize_local_aggregate_address_call_argument` plus
  `calls_dispatch_bridge.cpp:182` in
  `materialize_scalar_call_argument_value`: local aggregate address fallback.
  It is gated only by
  `PreparedCallArgumentPlan::allows_local_aggregate_address_publication`, then
  re-materializes an address from `local_aggregate_address_frame_offset`.
  Replacement authority should be the argument's
  `PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization`
  fields consumed through `make_selected_call_argument_source`; if those fields
  are absent or incomplete, dispatch should fail closed instead of publishing
  from the local frame offset.
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp:1187` in
  `materialize_missing_frame_slot_call_arguments`: already
  prepared-authoritative. It only consumes `FrameSlotAddress`,
  `FrameSlotValue`, or `PriorPreservation` source selections and silently skips
  when no prepared selected source exists; no owned fallback branch found here.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp:1434` in
  `selected_byval_lane_extent_bytes`: indirect byval lane payload fallback.
  When `source_selection` is absent, it derives the lane extent from
  `prepared_byval_lane_extent_bytes`; when selection is present but not
  `ByvalRegisterLane`, it may still derive from payload stores. The prepared
  fact should be `ByvalRegisterLane::byval_lane_extent_bytes`; absence or wrong
  selection kind should be unreachable for owned byval publication or produce a
  missing-authority diagnostic.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp:1824` and
  `calls_moves.cpp:2687`: indirect byval lane payload fallback for register
  call arguments. With absent selection, the path admits
  `prepared_indirect_byval_extent_bytes` and then builds a source through
  `make_byval_register_lane_prepared_source`. The prepared fact should be a
  complete `ByvalRegisterLane` source selection carrying source slot, stack
  offset, size, alignment, and lane extent; absent selection should be a precise
  missing selected-source condition.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp:2105` and
  `calls_moves.cpp:2263`: indirect byval lane payload fallback for register
  homes. If `make_byval_register_lane_prepared_source` has no selected source,
  and no payload stores or selected byval source are present, the code rebuilds
  a pointer-based memory source from the register home via
  `make_aggregate_call_argument_source`. The prepared fact should be complete
  `ByvalRegisterLane` selected source bytes; the fallback is the current
  compatibility route for absent selection.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp:2321` and
  `calls_moves.cpp:2412`: indirect byval lane payload fallback for fragmented
  register-lane publication. It falls back to
  `make_fragmented_aggregate_register_lane_publication_instruction` from
  payload stores when no selected source is available or selected bytes do not
  match payload stores. The prepared fact should be complete selected source
  bytes plus lane extent; mismatched or incomplete selected bytes should be a
  missing-authority condition, not BIR store reconstruction.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp:2790` through
  `calls_moves.cpp:2835`: indirect byval stack-lane payload fallback. It calls
  `collect_byval_register_lane_stores`, then may publish via
  `aggregate_lane_store_memory` when selected source bytes are unavailable.
  The prepared fact should be complete `ByvalRegisterLane` source selection;
  `collect_byval_register_lane_stores` should become unnecessary for this owned
  path after retirement.
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp:138`
  `collect_byval_register_lane_stores`: indirect byval lane payload fallback
  collector. It reconstructs payload lanes from BIR `load_local`/`store_local`
  names, prepared memory accesses, frame slots, and partial stores. This should
  be replaced by authoritative selected source bytes from
  `PreparedCallArgumentSourceSelectionKind::ByvalRegisterLane`; a missing
  selected source should be diagnosed by the callers.
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp:347`
  `make_byval_register_lane_prepared_source`: already
  prepared-authoritative. It succeeds only when `source_selection` is
  `ByvalRegisterLane` and carries matching lane extent, source slot, stack
  offset, size, and alignment.
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp:394`
  `aggregate_lane_store_memory`: indirect byval lane payload fallback adapter
  for stores returned by `collect_byval_register_lane_stores`; its precise
  unreachable condition after retirement is any byval publication reaching it
  without complete selected source bytes.
- `src/backend/mir/aarch64/codegen/calls_moves.cpp:2747`,
  `calls_moves.cpp:2781`, `calls_moves.cpp:2837`, `calls_moves.cpp:2848`,
  `calls_moves.cpp:2872`, `calls_moves.cpp:2916`, `calls_moves.cpp:2940`,
  `calls_moves.cpp:2977`, `calls_moves.cpp:2988`, `calls_moves.cpp:3024`,
  and `calls_moves.cpp:3093`: unrelated required diagnostics or already
  fail-closed checks for missing prepared source, byval size, destination stack
  offset, aggregate address register, f128 carriers, or scalar stack-slot
  sizes. They are not absent-selection compatibility fallbacks.

Focused tests proving current behavior:

- Local aggregate address fallback/guards:
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
  `missing_local_aggregate_frame_slot_call_argument_materializes_object_address`,
  `block_dispatch_publishes_direct_local_aggregate_address_call_argument`,
  `block_dispatch_publishes_zero_offset_local_aggregate_address_call_argument`,
  `block_dispatch_keeps_byval_aggregate_argument_out_of_local_address_fallback`,
  and `block_dispatch_keeps_va_start_intrinsic_out_of_local_address_fallback`.
- Prepared local-frame selection facts:
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  `check_local_frame_address_source_selection_contract` and
  `check_missing_local_aggregate_frame_slot_address_source_selection_contract`.
- Byval lane prepared/fallback behavior:
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
  `prepared_small_byval_aggregate_call_argument_loads_prepared_payload_lane`,
  `register_home_small_byval_aggregate_call_argument_loads_payload_slots`,
  `stack_home_two_byte_byval_aggregate_call_argument_loads_payload_slots`,
  `incomplete_byval_register_lane_selection_does_not_rederive_register_home`,
  `incomplete_byval_stack_lane_selection_does_not_rederive_frame_slot_home`,
  `incomplete_byval_stack_register_lane_selection_does_not_rederive_frame_slot_home`,
  `overflow_byval_aggregate_call_argument_publishes_prepared_stack_lanes`, and
  `large_byval_aggregate_indirect_argument_materializes_frame_address`.
- Boundary-owner byval smoke coverage:
  `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`
  `byval_caller_publishes_composite_gpr_lanes_not_object_pointer`.

## Suggested Next

Supervisor should delegate Step 2 local aggregate address fallback retirement.
The narrow packet should make dispatch consume prepared
`LocalFrameAddressMaterialization` facts instead of
`local_aggregate_address_frame_offset`, preserving the positive local aggregate
tests and adding or updating the missing-fact diagnostic expectation.

## Watchouts

- Do not edit the source idea for routine audit findings.
- Do not treat expectation downgrades or fallback renames as progress.
- Keep `review/` artifacts transient.
- The byval family has both register and stack-lane consumers; retiring only
  one branch would leave `collect_byval_register_lane_stores` available as an
  alternate absent-selection path.
- Some existing tests intentionally prove legacy absent-selection
  compatibility. Step 2/Step 3 should convert those to prepared-authority or
  fail-closed expectations only after the implementation facts exist.

## Proof

No build/test proof required by the packet. Ran lightweight source inspection
only with `sed`, `nl`, `rg`, and `git status --short`; no `test_after.log`
updated for this audit-only slice.
