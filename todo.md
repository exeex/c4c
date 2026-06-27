# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Typed FrameSlotValue Payload and Bridge Accessor

## Just Finished

Completed Step 2 typed payload/accessor work for `FrameSlotValue`.

Added `PreparedCallArgumentFrameSlotValueRoute` and
`as_frame_slot_value_source_route` in `src/backend/prealloc/calls.hpp` without
adding optional fields to `PreparedCallArgumentSourceSelection`.

The typed query requires source value id, source value name, stack-slot
source-home kind, source slot, source stack byte offset, source extent, and
source alignment. It rejects non-stack source homes, address-materialization
payloads, source-base/pointer-delta payloads, preservation payloads, byval-lane
payloads, and missing required facts.

`find_prepared_missing_frame_slot_call_argument_publication_need` now requires
the typed query before classifying `FrameSlotValue`. Focused coverage in
`backend_prepare_frame_stack_call_contract_test.cpp` checks valid typed route
exposure, rejected invalid bags, and publication-bridge visibility.

Updated `docs/prepared_fact_contracts/call_argument_contract_plan.md` with the
`FrameSlotValue` Step 2 route scope.

## Suggested Next

Begin Step 3 by adding producer-side verifier statuses and reports for
`FrameSlotValue`.

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
  bag; Step 4 owns that migration after verifier coverage exists.

## Proof

Passed delegated proof in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`

Result: 16/16 selected tests passed.
