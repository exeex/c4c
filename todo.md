# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Typed Route Payloads and Bridge Accessors

## Just Finished

Completed Step 2 for the first migrated route. Added
`PreparedCallArgumentFrameSlotAddressRoute` plus
`as_frame_slot_address_source_route` as the typed compatibility bridge for
`PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`, without adding new
optional fields to `PreparedCallArgumentSourceSelection`.

The typed bridge exposes only the frame-slot address route facts: source slot,
source stack byte offset, extent, alignment, optional source value/home identity,
and an optional complete address-materialization subpayload. It rejects missing
required facts, partial materialization payloads, materialization slot/source
slot contradictions, non-stack source-home kinds, preservation payloads,
byval-lane payloads, source-base payloads, and pointer-delta payloads.

`find_prepared_missing_frame_slot_call_argument_publication_need` now uses the
typed route query for `FrameSlotAddress`, so contradictory old optional-bag
combinations are not visible through the shared missing frame-slot publication
bridge. Focused coverage in
`backend_prepare_frame_stack_call_contract_test.cpp` checks valid typed route
exposure, rejected contradictions, and publication-bridge visibility.

Created `docs/prepared_fact_contracts/call_argument_contract_plan.md` with the
Step 2 route scope and consumed rows:
`TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001`,
`TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001`, call-route portions of
`TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001`, storage/home portions of
`TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`,
`418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001`, and
`418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`.

## Suggested Next

Begin Step 3 by adding producer-side verification/diagnostics for
`FrameSlotAddress` route requirements and contradictions, using the typed route
query as the shared validity boundary.

## Watchouts

- Do not add new optional fields to `PreparedCallArgumentSourceSelection`.
- Do not infer argument homes in RV64/AArch64 when producer facts are absent.
- Do not use named torture-file handling or expectation weakening as progress.
- Step 2 currently makes invalid `FrameSlotAddress` bags absent from typed
  publication classification; Step 3 should turn those same failures into
  producer-owned diagnostics instead of silent target omission.
- Existing producers still write the compatibility bag, so Step 3 should verify
  both generated selections and any direct helper inputs before target lowering.
- `ByvalRegisterLane`, `FrameSlotValue`, `LocalFrameAddressMaterialization`, and
  `PriorPreservation` remain unmigrated route families.

## Proof

Passed delegated proof in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`

Result: 16/16 selected tests passed.
