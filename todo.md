# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate Target Consumers for FrameSlotValue

## Just Finished

Completed Step 4 for the first migrated `FrameSlotValue` consumers.

AArch64 `StackFrameSlotCallOperandOwner::selected_frame_slot_source` now
verifies and consumes `PreparedCallArgumentFrameSlotValueRoute` for
`FrameSlotValue` instead of reading the compatibility bag directly.

RV64 object emission `prepared_frame_slot_call_argument_offset` now verifies
`FrameSlotValue` selections with
`verify_prepared_frame_slot_value_source_route_contract`, consumes
`as_frame_slot_value_source_route`, and checks typed route value/slot/offset
facts against prepared value-home data before selecting the stack source.

The call-argument contract plan now records the `FrameSlotValue` Step 4
consumer migration scope.

## Suggested Next

Begin Step 5 by running broad validation and deciding the next route candidate.

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
- Step 4 only migrates selected `FrameSlotValue` consumers found in Step 1; do
  not infer that other route families are typed.
- The next route candidate should likely be `PriorPreservation` or
  `LocalFrameAddressMaterialization`; defer the choice to Step 5 after broad
  validation.

## Proof

Passed delegated proof in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`

Result: 16/16 selected tests passed.
