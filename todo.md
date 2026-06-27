# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Typed Local Materialization Payload and Bridge Accessor

## Just Finished

Completed Step 2 typed payload/accessor work for
`LocalFrameAddressMaterialization`.

Added `PreparedCallArgumentLocalFrameAddressMaterializationRoute` and
`as_local_frame_address_materialization_route` in
`src/backend/prealloc/calls.hpp` without adding optional fields to
`PreparedCallArgumentSourceSelection`.

The typed query requires source value id/name, source-home kind of `Register`
or `PointerBasePlusOffset`, source pointer byte delta, source slot, source
stack offset, source extent/alignment, and complete adjusted address
materialization facts. It rejects negative adjusted materialization offsets,
slot/offset contradictions, preservation payloads, byval-lane payloads, and
missing required facts.

`find_prepared_missing_frame_slot_call_argument_publication_need` now requires
the typed query before classifying `LocalFrameAddressMaterialization`. Focused
coverage in `backend_prepare_frame_stack_call_contract_test.cpp` checks valid
typed route exposure, rejected invalid bags, and publication-bridge visibility.

Updated `docs/prepared_fact_contracts/call_argument_contract_plan.md` with the
local-materialization Step 2 route scope.

## Suggested Next

Begin Step 3 by adding producer-side verifier statuses and reports for
`LocalFrameAddressMaterialization`.

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

Selected delegated proof passed: 16/16 tests, with monotonic regression guard
PASS against the matching 16/16 baseline. Proof command:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prealloc_call_boundary_classification$|backend_prepare_frame_stack_call_contract$|backend_riscv_object_emission$|backend_aarch64_call_boundary_owner$|backend_(dump|codegen_route)_riscv64_byval_|backend_codegen_route_aarch64_(prepared_call_boundary_scalability|alu_unpublished_load_local_after_call|alu_unpublished_load_local_call_boundary|hfa_result_home_publication_contract)$)' ) > test_after.log 2>&1`
