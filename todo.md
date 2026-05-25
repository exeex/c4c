Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: retired the AArch64-local before-call
preservation population selection gate that rechecked prior preserved values
after `prepare::plan_prepared_call_boundary_effects` had already produced
ordered `PreservationHomePopulation` effects.

Concrete cleanup:

- `lower_before_call_moves` now consumes the prepared preservation boundary
  effect stream directly and leaves `make_callee_saved_preservation_home_population`
  responsible only for AArch64 machine-node emission.
- Deleted the now-unused `find_prior_preserved_value_for_value` helper and its
  `calls.hpp` declaration.
- Kept `find_prior_preserved_value_for_call_argument` intact because it still
  backs argument-source preservation reuse and is outside this packet.

## Suggested Next

Execute the next Step 2 preservation slice by removing one remaining
effect-shaping wrapper around republication, only if the after-call and
block-entry paths can still preserve their distinct emitted phases without
moving planning back into AArch64-local code.

## Watchouts

- `make_callee_saved_preservation_home_republication_instruction` is shared by
  after-call republication and block-entry republication; do not collapse its
  phase parameters unless both call sites remain behavior-preserving.
- Local-frame address publication and byval register-lane helpers remain
  outside the clean deletion target set until a shared source-address or lane
  materialization fact exists.
- No tests were weakened or rewritten for this slice.

## Proof

Ran delegated proof successfully:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build passed; backend CTest subset passed, 162/162 tests.

Proof log: `test_after.log`.
