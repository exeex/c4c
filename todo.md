Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` removed the duplicate
`collect_byval_register_lane_stores` reconstruction tail from
`make_byval_register_lane_prepared_source`, leaving the helper to accept only a
complete prepared `ByvalRegisterLane` source-selection payload and otherwise
fail closed.

The hand-built AArch64 instruction-dispatch byval fixtures now provide the
prepared `ByvalRegisterLane` source-selection facts that production prepared
call planning supplies. The small byval payload fixture selects
`PreparedFrameSlotId{13203}` at stack offset `128` with size/alignment `1` and
lane extent `1`; the large indirect byval fixture selects
`PreparedFrameSlotId{43}` at stack offset `96` with size `24`, alignment `8`,
and lane extent `24`.

## Suggested Next

Continue Step 2 by retiring the next proven duplicate helper boundary only if
its prepared source-selection payload is complete and its hand-built fixtures
already model the prepared contract.

## Watchouts

- Do not retire fragmented byval lane fallback logic outside
  `make_byval_register_lane_prepared_source`; the accepted prerequisite only
  covers complete byval register-lane selections with prepared source payload.
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
