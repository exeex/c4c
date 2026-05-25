Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: retired the AArch64-local after-call
preservation republication wrapper that only reshaped prepared
`PreservationRepublication` effects before handing them to the shared
emission helper.

Concrete cleanup:

- Deleted the anonymous `make_callee_saved_preservation_home_republication`
  wrapper in `calls_moves.cpp`.
- `lower_after_call_moves` now calls
  `make_callee_saved_preservation_home_republication_instruction` directly with
  the same `BeforeInstruction` phase, call-plan block/instruction indexes, and
  reason fallback used by the wrapper.
- Left block-entry republication emission unchanged; it still calls the same
  emission helper with `BlockEntry` phase and its block-entry reason.

## Suggested Next

Continue Step 2 by checking whether another narrow call-boundary helper can be
reduced to direct consumption of prepared facts without changing emitted move
phase, origin, or reason spelling.

## Watchouts

- `make_callee_saved_preservation_home_republication_instruction` remains the
  shared emission boundary for both after-call and block-entry republication;
  keep the phase/reason arguments explicit at each call site.
- Local-frame address publication and byval register-lane helpers remain
  outside the clean deletion target set until a shared source-address or lane
  materialization fact exists.
- No tests were weakened or rewritten for this slice.

## Proof

Ran delegated proof successfully:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build passed; backend CTest subset passed, 162/162 tests.

Proof log: `test_after.log`.
