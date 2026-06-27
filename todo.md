# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Producer-Side Verification

## Just Finished

Completed Step 3 producer-side verification for `FrameSlotValue`.

Added `PreparedFrameSlotValueSourceRouteContractStatus`,
`classify_prepared_frame_slot_value_source_route_contract`, and
`verify_prepared_frame_slot_value_source_route_contract` using
`PreparedContractFactFamily::CallArgumentTypedRoute`.

The verifier classifies missing selected `FrameSlotValue` route facts as
`producer_missing`: absent route, source value id, source value name,
source-home kind, source slot, source stack offset, extent, and alignment. It
classifies contradictory payloads as `producer_incoherent`: non-stack
source-home kind, address-materialization payloads, and mixed preservation,
byval, source-base, or pointer-delta payloads.

Focused verifier coverage in
`backend_prealloc_prepared_contract_verifier_test.cpp` checks coherent,
producer-missing, and producer-incoherent reports plus status spelling. The
call-argument contract plan now records the `FrameSlotValue` Step 3 verifier
statuses.

## Suggested Next

Begin Step 4 by migrating RV64/AArch64 `FrameSlotValue` consumers to use
`as_frame_slot_value_source_route` plus the Step 3 verifier.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not disturb the completed `FrameSlotAddress` route except where a shared
  helper needs a behavior-preserving cleanup.
- `FrameSlotValue` should require source value/home identity plus source slot,
  stack offset, extent, and alignment.
- Keep `FrameSlotAddress`, `PriorPreservation`,
  `LocalFrameAddressMaterialization`, and `ByvalRegisterLane` route behavior
  unchanged unless the active step explicitly owns a shared boundary.
- Do not preserve the old typed-query behavior that accepts only slot or only
  stack offset; that compatibility should remain outside the typed API.
- RV64/AArch64 consumers still read `FrameSlotValue` through the compatibility
  bag; Step 4 owns that migration.

## Proof

Passed delegated proof in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`

Result: 16/16 selected tests passed.
