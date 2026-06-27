# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Producer-Side Verification

## Just Finished

Completed Step 3 producer-side verification for the first migrated route.
Added `PreparedContractFactFamily::CallArgumentTypedRoute`,
`PreparedFrameSlotAddressSourceRouteContractStatus`,
`classify_prepared_frame_slot_address_source_route_contract`, and
`verify_prepared_frame_slot_address_source_route_contract`.

The verifier classifies missing selected `FrameSlotAddress` route facts as
`producer_missing`: absent route, missing source slot, missing source stack
offset, missing extent, and missing alignment. It classifies contradictory
payloads as `producer_incoherent`: non-stack source-home kind, partial address
materialization, materialization slot/source slot mismatch, and mixed
preservation/byval/source-base/pointer-delta payload.

Focused verifier coverage in
`backend_prealloc_prepared_contract_verifier_test.cpp` checks coherent,
producer-missing, and producer-incoherent reports plus the typed-route fact
family spelling. The call-argument contract plan now records the Step 3
producer-owned status boundary.

## Suggested Next

Begin Step 4 by migrating RV64/AArch64 `FrameSlotAddress` consumers to require
the typed route and consume the Step 3 verifier result at fail-closed sites
instead of reconstructing missing route facts.

## Watchouts

- Do not add new optional fields to `PreparedCallArgumentSourceSelection`.
- Do not infer argument homes in RV64/AArch64 when producer facts are absent.
- Do not use named torture-file handling or expectation weakening as progress.
- Existing producers still write the compatibility bag, so Step 4 should keep
  using the typed bridge/verifier instead of reading bag fields directly in
  target consumers.
- Step 3 provides a verifier report but does not yet attach it to every
  RV64/AArch64 target fail-closed path; that is the Step 4 migration.
- `ByvalRegisterLane`, `FrameSlotValue`, `LocalFrameAddressMaterialization`, and
  `PriorPreservation` remain unmigrated route families.

## Proof

Passed delegated proof in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`

Result: 16/16 selected tests passed.
