# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Target Consumers

## Just Finished

Completed Step 4 consumer migration for
`LocalFrameAddressMaterialization`.

`plan_prepared_aggregate_transport`, RV64 call emission, RV64 object-emission
address validation, and AArch64 selected local frame-address source handling now
consume `as_local_frame_address_materialization_route` instead of recovering
local materialization fields directly from the compatibility bag.

AArch64 selected local frame-address source handling also runs
`verify_prepared_local_frame_address_materialization_source_route_contract`
before constructing the prepared memory operand.

Updated hand-authored RV64 prepared fixtures to publish coherent
local-materialization source-home, pointer-delta, and materialization payload
facts.

## Suggested Next

Begin Step 5 by running broader default validation and recording the next route
decision or lifecycle review request.

## Watchouts

- Do not add optional fields to `PreparedCallArgumentSourceSelection`.
- Do not merge `LocalFrameAddressMaterialization` with `ByvalRegisterLane`
  unless lifecycle review creates a separate coupling plan.
- Preserve completed `FrameSlotAddress` and `FrameSlotValue` typed contracts.
- The previous broad validation before regeneration was 3356/3356 passing.
- Keep byval aggregate transport as a consumer of the local-materialization
  route; do not absorb byval register-lane payloads into this typed route.
- RV64/AArch64 consumers still read this route through the compatibility bag;
  Step 4 owns that migration after verifier coverage exists.

## Proof

Selected delegated proof passed: 17/17 tests, with monotonic regression guard
PASS against the matching 17/17 baseline. Proof command:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_prealloc_prepared_contract_verifier$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`
