Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Retire One Proven Duplicate Helper Boundary

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: retired the AArch64-local boundary-effect
wrapper layer that only repackaged prepared call-boundary effects before
emission loops consumed them.

Concrete cleanup:

- Deleted the anonymous `before_call_boundary_effects` helper in
  `calls_moves.cpp`; `lower_before_call_moves` now calls
  `prepare::plan_prepared_call_boundary_effects` directly and consumes the
  prepared population/explicit-move effects.
- Deleted the anonymous `after_call_explicit_boundary_effects` helper in
  `calls_moves.cpp`; `lower_after_call_moves` now calls the prepared planner
  directly and filters explicit `AfterCall` effects at the emission loop.
- Kept AArch64 code limited to prepared-effect filtering plus machine-node
  emission; the prepared planner remains the owner of effect ordering and
  call-plan indexes.

## Suggested Next

Continue Step 2 only if the supervisor wants another narrow helper scan; the
remaining obvious local-frame address and byval lane helpers still need a
shared prepared source-address or lane-materialization fact before they are
clean deletion targets.

## Watchouts

- `make_callee_saved_preservation_home_republication_instruction` remains the
  shared emission boundary for both after-call and block-entry republication;
  keep the phase/reason arguments explicit at each call site.
- Local-frame address publication and byval register-lane helpers remain
  outside the clean deletion target set until a shared source-address or lane
  materialization fact exists.
- `find_prior_preserved_value_for_call_argument` and
  `make_prior_preserved_call_argument_source` are still required by the
  preserved-argument source reuse path; deleting them would require an explicit
  prepared call-argument source fact for that source selection.
- No tests were weakened or rewritten for this slice.

## Proof

Ran delegated proof successfully:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build passed; backend CTest subset passed, 162/162 tests.

Proof log: `test_after.log`.
