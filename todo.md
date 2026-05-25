Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` retired the explicit `FrameSlotValue` fallback inside
`make_frame_slot_call_argument_source`: when a prepared source-selection fact
claims `PreparedCallArgumentSourceSelectionKind::FrameSlotValue`, the helper now
accepts only the complete prepared payload through
`make_selected_frame_slot_source` and otherwise fails closed instead of
rederiving the same frame-slot value source from local addressing/home facts.

The existing hand-built AArch64 instruction-dispatch frame-slot stack argument
fixture now provides the prepared `FrameSlotValue` source-selection facts that
production prepared call planning supplies: value `PreparedValueId{93}` /
`ValueNameId{94}` selects `PreparedFrameSlotId{5}` at stack offset `32` with
size/alignment `8`.

## Suggested Next

Continue Step 2 by retiring the next proven duplicate helper boundary, likely a
`FrameSlotAddress`, `LocalFrameAddressMaterialization`, or stack-slot
`PriorPreservation` path, only where the prepared source-selection payload is
complete and the fallback can be separated from incomplete/absent selection
compatibility.

## Watchouts

- Do not retire fragmented byval lane fallback logic outside
  `make_byval_register_lane_prepared_source`; the accepted prerequisite only
  covers complete byval register-lane selections with prepared source payload.
- `make_frame_slot_call_argument_source` still keeps the legacy fallback when
  no explicit `FrameSlotValue` selection is present. A full fail-closed removal
  crossed into an existing byval-sized hand fixture with no frame-slot
  source-selection fact, so absent-selection compatibility remains outside this
  packet.
- Do not retire callee-saved-register prior preservation in the same slice;
  `PreparedCallArgumentSourceSelection` only proves the stack-slot
  `PriorPreservation` source path for this consolidation step.
- Frame-slot value/address and local-frame address materialization fallbacks
  are also mapped, but they have broader helper interactions and should follow
  after the byval slice proves the pattern.
- Do not invent a new call-plan API under this source idea.
- Do not move AArch64 emission details into the shared planner.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.

## Proof

Ran delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } | tee test_after.log'`

Result: build succeeded and the backend subset passed, 162/162 tests.

Proof log: `test_after.log`.
