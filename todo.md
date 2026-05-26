Status: Active
Source Idea Path: ideas/open/17_aarch64_absent_selection_fallback_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Normalize Legacy Fallback Tests

# Current Packet

## Just Finished

Step 4: Normalize Legacy Fallback Tests completed as an audit/no-op
normalization packet. The Step 1 fallback-related tests in the owned files now
map as follows:

- Positive prepared local-frame address behavior:
  `backend_aarch64_instruction_dispatch_test.cpp`
  `missing_local_aggregate_frame_slot_call_argument_materializes_object_address`,
  `block_dispatch_publishes_direct_local_aggregate_address_call_argument`, and
  `block_dispatch_publishes_zero_offset_local_aggregate_address_call_argument`
  consume explicit `FrameSlotAddress` or `LocalFrameAddressMaterialization`
  source selections and assert `add xN, sp, #offset` publication from the
  selected frame object instead of a spill or legacy lookup.
- Missing/incomplete local-frame selected-source diagnostics:
  `incomplete_frame_slot_address_selection_does_not_rederive_value_source`,
  `absent_frame_slot_address_selection_does_not_rederive_legacy_value_source`,
  and `incomplete_local_frame_address_selection_does_not_rederive_legacy_lookup`
  require `MissingValueAuthority` or the prepared
  `LocalFrameAddressMaterialization` diagnostic rather than source
  rederivation.
- Local-address unrelated guards:
  `block_dispatch_keeps_byval_aggregate_argument_out_of_local_address_fallback`
  and `block_dispatch_keeps_va_start_intrinsic_out_of_local_address_fallback`
  remain guard coverage for non-local-aggregate ABI arguments and intentionally
  assert that those shapes do not enter the local-address publication route.
- Positive prepared byval behavior:
  `prepared_small_byval_aggregate_call_argument_loads_prepared_payload_lane`,
  `register_home_small_byval_aggregate_call_argument_loads_payload_slots`,
  `stack_home_two_byte_byval_aggregate_call_argument_loads_payload_slots`,
  `overflow_byval_aggregate_call_argument_publishes_prepared_stack_lanes`,
  `large_byval_aggregate_indirect_argument_materializes_frame_address`, and
  `backend_aarch64_call_boundary_owner_test.cpp`
  `byval_caller_publishes_composite_gpr_lanes_not_object_pointer` use complete
  `ByvalRegisterLane` selected-source facts or the remaining intended indirect
  frame-address path and assert payload/address semantics instead of broad
  BIR-store rediscovery.
- Missing/incomplete byval selected-source diagnostics:
  `incomplete_byval_register_lane_selection_does_not_rederive_register_home`,
  `incomplete_byval_stack_lane_selection_does_not_rederive_frame_slot_home`,
  `incomplete_byval_stack_register_lane_selection_does_not_rederive_frame_slot_home`,
  and the indirect byval absent/incomplete checks in
  `large_byval_aggregate_indirect_argument_materializes_frame_address` fail
  closed with `MissingValueAuthority` rather than deriving payload bytes from
  the register home, frame-slot home, or old payload-store reconstruction.
- Prepared-front-end contract coverage:
  `backend_prepare_frame_stack_call_contract_test.cpp`
  `check_local_frame_address_source_selection_contract` and
  `check_missing_local_aggregate_frame_slot_address_source_selection_contract`
  assert that the prepared call plan publishes
  `LocalFrameAddressMaterialization` facts. These are positive prepared-fact
  contracts, not legacy fallback expectations.

No owned test still expects the retired local-aggregate address fallback or the
retired indirect byval lane payload fallback. No test edit was needed.

## Suggested Next

Supervisor should advance to Step 5 validation/closure review for the active
fallback-retirement route.

## Watchouts

- Do not edit the source idea for routine audit findings.
- Do not treat expectation downgrades or fallback renames as progress.
- Keep `review/` artifacts transient.
- `clang-format` is not installed in this environment; formatting was kept
  manual.
- `calls_byval_aggregates.cpp` is now intentionally small; remaining byval
  authority is the selected-source adapter, not BIR payload-store analysis.
- This packet did not rename guard tests containing `fallback` in their names;
  those names describe exclusion from the retired route, not a supported legacy
  expectation.

## Proof

Ran the delegated proof:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner)$"'`.
Result: passed. Proof log: `test_after.log`.
