# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Producer-Side Verification

## Just Finished

Completed Step 3 producer-side verifier coverage for
`LocalFrameAddressMaterialization`.

Added `PreparedLocalFrameAddressMaterializationSourceRouteContractStatus`,
status spelling, classifier, and
`verify_prepared_local_frame_address_materialization_source_route_contract` in
`src/backend/prealloc/prepared_contract_verifier.*`.

Missing source identity, source home, pointer base identity when required,
pointer delta, source slot, stack offset, extent/alignment, and materialization
location/slot/offset facts classify as `producer_missing`.

Wrong source-home kind, negative materialization offset, materialization
slot/offset contradictions, and preservation/byval-lane cross-route payloads
classify as `producer_incoherent`.

Focused coverage in
`tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp` checks
coherent, missing, and incoherent route reports.

## Suggested Next

Begin Step 4 by migrating RV64/AArch64 consumers to the typed local
materialization route plus verifier checks.

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
PASS against the matching 16/16 baseline. Proof command:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_prealloc_prepared_contract_verifier$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`
