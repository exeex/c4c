Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` retired the explicit
`PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization`
fallback boundary for AArch64 local-frame address call arguments. When a call
argument carries this explicit selection kind, the helper now consumes the
selected local-frame address payload only if it is complete and no longer
rederives the same address through prepared address materialization or
stack-object lookup.

The instruction-dispatch coverage now includes an incomplete explicit
local-frame address selection while matching legacy prepared addressing and
stack-object facts are still present; `lower_before_call_moves` emits no
rederived x0 address move. Existing no-selection local-frame address coverage
still proves absent `source_selection` compatibility through the legacy lookup.

## Suggested Next

Continue Step 2 by auditing the remaining selected-source helpers for another
complete-payload-only duplicate boundary that can be retired without touching
absent-selection compatibility. A good next packet is the remaining
frame-slot value source surface that still keeps explicit incomplete
selection handling adjacent to legacy compatibility.

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
- Frame-slot address and local-frame address materialization complete
  selected-source paths are now consumed from prepared facts, and explicit
  incomplete selections fail closed. Absent-selection compatibility still uses
  legacy lookup.
- Stack-slot `PriorPreservation` explicit selections now require a complete
  stack payload; absent-selection prior-preservation lookup remains the
  compatibility path.
- SRET `FrameSlotAddress` explicit selections now require a complete
  frame-slot address payload; absent-selection SRET compatibility still uses
  `call_plan.memory_return`.
- Do not invent a new call-plan API under this source idea.
- Do not move AArch64 emission details into the shared planner.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.

## Proof

Ran delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } | tee test_after.log'`

Result: build succeeded and the backend subset passed, 162/162 tests.

Proof log: `test_after.log`.
