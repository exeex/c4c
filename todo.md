Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` retired the next proven duplicate source boundary for
`PreparedCallArgumentSourceSelectionKind::LocalFrameAddressMaterialization`
where the prepared materialization payload is complete: AArch64 now reads the
selected local-frame address source directly from the prepared materialization
fields without requiring the legacy source stack fields or rederiving the
frame address from local address-materialization/stack-object scans.

The local-address instruction-dispatch fixture now supplies a complete prepared
`LocalFrameAddressMaterialization` source-selection fact and no longer provides
the old stack-object fallback, proving the selected-source path is consumed.
Incomplete or absent local-frame address selections still fall through to the
existing compatibility fallback.

## Suggested Next

Continue Step 2 by retiring the stack-slot `PriorPreservation` helper boundary
only where `PreparedCallArgumentSourceSelectionKind::PriorPreservation`
contains a complete stack-slot preservation payload. Keep callee-saved-register
prior preservation and incomplete/absent selection compatibility outside that
packet.

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
- Frame-slot value/address and local-frame address materialization complete
  selected-source paths are now consumed from prepared facts. Local-frame
  address materialization still intentionally preserves compatibility fallback
  for incomplete/absent selections.
- Do not invent a new call-plan API under this source idea.
- Do not move AArch64 emission details into the shared planner.
- Do not claim progress through file concatenation, expectation weakening, or
  hidden helper rewrites.

## Proof

Ran delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } | tee test_after.log'`

Result: build succeeded and the backend subset passed, 162/162 tests.

Proof log: `test_after.log`.
