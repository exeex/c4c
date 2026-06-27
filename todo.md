# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Target Consumers for the Selected Route

## Just Finished

Completed Step 4 for the first migrated `FrameSlotAddress` consumers. RV64
`verified_prepared_selected_frame_slot_address_offset` now verifies and consumes
`PreparedCallArgumentFrameSlotAddressRoute` before reading route facts, and
`emit_riscv_frame_slot_address_argument` no longer falls back to target-side
address-materialization scanning when the typed source selection is absent.

AArch64 `StackFrameSlotCallOperandOwner::selected_frame_slot_source` now
verifies and consumes `PreparedCallArgumentFrameSlotAddressRoute` for
`FrameSlotAddress`. The legacy optional-bag path remains only for unmigrated
`FrameSlotValue` handling.

The call-argument contract plan now records the Step 4 RV64/AArch64 migration
scope and keeps the next route decision deferred to Step 5.

## Suggested Next

Begin Step 5 by running the supervisor-selected broad validation and deciding
the next route candidate.

## Watchouts

- Do not add new optional fields to `PreparedCallArgumentSourceSelection`.
- Do not infer argument homes in RV64/AArch64 when producer facts are absent.
- Do not use named torture-file handling or expectation weakening as progress.
- Existing producers still write the compatibility bag for unmigrated route
  families; do not generalize the FrameSlotAddress bridge to those routes until
  their typed payloads exist.
- Step 4 intentionally leaves sret memory-return fallback and unmigrated
  `FrameSlotValue`/preservation/byval paths in place; Step 5 should decide the
  next route rather than broadening this slice.
- `ByvalRegisterLane`, `FrameSlotValue`, `LocalFrameAddressMaterialization`, and
  `PriorPreservation` remain unmigrated route families.

## Proof

Passed delegated proof in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`

Result: 16/16 selected tests passed.
