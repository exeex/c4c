Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Surviving Republication Helper Boundary

# Current Packet

## Just Finished

Step 3 - Consolidate The Surviving Republication Helper Boundary: removed the
dead AArch64-local non-call-use selection helpers and their public declarations
after block-entry callee-saved preservation republication selection moved into
prepared call-plan lookups.

The surviving
`make_callee_saved_preservation_home_republication_instruction` boundary is now
documented as emission-only: it consumes prepared effect facts and emits AArch64
machine nodes, while prepared lookups own eligibility and block-entry
reachability selection.

## Suggested Next

Delegate lifecycle review or the next implementation packet from the supervisor
to decide whether this Step 3 closeout is sufficient to advance the route.

## Watchouts

- The prepared lookup non-call-use helpers remain in `prepared_lookups.cpp`
  because they are the shared selection boundary for block-entry republication.
- The AArch64 republication helper still validates effect shape and prepared
  storage facts before emission; it does not walk call plans or scan BIR uses.
- No expectation downgrades, call-printing changes, or source-idea/plan edits
  were made.

## Proof

Proof passed:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

`test_after.log` reports 162 backend tests passed, 0 failed.
