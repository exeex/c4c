Status: Active
Source Idea Path: ideas/open/06_prepared_call_preservation_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Redirect AArch64 Emission To Prepared Authority

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: redirected AArch64 prior-preserved call
argument lowering to consume prepared `PriorPreservation` source-selection
records in the hand-built AArch64 instruction-dispatch fixtures.

`nested_call_argument_publishes_from_prior_preservation_home` now publishes an
explicit prepared callee-saved `PriorPreservation` source selection from the
first call's preserved `x20` home before asserting the later argument move.
The adjacent cross-block and stack-preserved fixture variants now publish the
same prepared authority instead of relying on absent-selection rediscovery, and
the local-frame-address negative checks now require the missing-authority
diagnostic rather than fallback to prior-preservation homes.

## Suggested Next

Supervisor can review and commit the Step 4 code plus `todo.md` slice, or run
any broader milestone validation it wants before accepting the completed step.

## Watchouts

- `clang-format` is not installed in this environment; formatting was kept to
  the existing local style manually.
- The dispatch fixture had several related absent-selection expectations in
  the same prior-preservation cluster; all failures observed by the delegated
  backend proof are now corrected.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. The build passed and the backend CTest subset passed 162/162.
Proof log: `test_after.log`.
