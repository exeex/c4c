# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broaden Validation and Decide the Next Route

## Just Finished

Completed Step 5 broad validation for the migrated `FrameSlotAddress` route.
Ran default CTest after the typed route payload, producer-side verifier, and
RV64/AArch64 consumer migration slices.

Validation result: 3356/3356 tests passed.

Next route candidate: `FrameSlotValue`. It shares the selected frame-slot
storage facts and AArch64/RV64 consumer surface with `FrameSlotAddress`, but is
narrower than `PriorPreservation`, `LocalFrameAddressMaterialization`, or
`ByvalRegisterLane`.

## Suggested Next

Lifecycle review: the runbook completed all five steps for the selected
`FrameSlotAddress` route. The plan owner should decide whether to close this
idea as a completed first-route migration or regenerate/split for the next
route candidate.

## Watchouts

- Do not add new optional fields to `PreparedCallArgumentSourceSelection`.
- Do not infer argument homes in RV64/AArch64 when producer facts are absent.
- Do not use named torture-file handling or expectation weakening as progress.
- Existing producers still write the compatibility bag for unmigrated route
  families; do not generalize the FrameSlotAddress bridge to those routes until
  their typed payloads exist.
- `FrameSlotValue` should be the next candidate if this idea remains active:
  required facts are selected source value/home identity, source slot, source
  stack byte offset, source extent, and source alignment.
- `ByvalRegisterLane`, `FrameSlotValue`, `LocalFrameAddressMaterialization`, and
  `PriorPreservation` remain unmigrated route families.

## Proof

Passed broad validation in `test_after.log`:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure ) > test_after.log 2>&1`

Result: 3356/3356 tests passed.
