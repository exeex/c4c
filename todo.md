Status: Active
Source Idea Path: ideas/open/07_aarch64_call_boundary_move_emission_only.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Coverage For Complete Source-Kind Emission

# Current Packet

## Just Finished

Step 2 focused coverage completed for complete prepared source-kind emission.

Tightened `backend_aarch64_instruction_dispatch_test.cpp` so complete
`FrameSlotValue` and `FrameSlotAddress` source selections emit from the
selected source bytes/address even when legacy `PreparedValueHome` and
argument source fields are poisoned to different frame slots and offsets.

Existing focused coverage in the delegated subset also proves complete
prepared selections for prior preservation, local frame-address
materialization, and byval register lanes with complete selected source bytes.
No production code was changed.

## Suggested Next

Start the next implementation packet that redirects AArch64 source-kind
selection branches to `PreparedCallArgumentSourceSelection::kind` for the
covered complete source kinds.

## Watchouts

- Keep source-selection authority in prepared call facts, not AArch64 emission.
- Do not touch the transient `review/` artifacts unless explicitly delegated.
- Treat expectation weakening or named-test shortcuts as route failures.
- Byval lane payload-store discovery is still only acceptable when selected
  source bytes are complete; otherwise report the missing prepared facts.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_memory_operand_records|backend_aarch64_operand_resolution)$'`

Passed. Proof log: `test_after.log`.
