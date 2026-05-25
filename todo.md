Status: Active
Source Idea Path: ideas/open/05_prepared_call_argument_source_selection_completeness.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Add Focused Coverage

# Current Packet

## Just Finished

Completed Step 4 for `plan.md`: closed the reviewer-identified AArch64
explicit callee-saved prior-preservation gap by treating an explicit
`PriorPreservation` selection with route `CalleeSavedRegister` as authoritative.
If its structured register payload is incomplete, lowering now fails closed
instead of falling back through `find_prior_preserved_value_for_call_argument`.

Added focused backend dispatch coverage for the incomplete explicit
callee-saved selection behavior, and updated the positive explicit callee-saved
prior-preservation fixture to carry complete register payload facts.

## Suggested Next

Supervisor should decide whether Step 4 coverage is now sufficient for reviewer
re-check, broader milestone validation, or lifecycle closure work.

## Watchouts

The new fail-closed rule is scoped to explicit `PriorPreservation` selections
with `CalleeSavedRegister` route. Local-frame-address selections still use their
existing guarded prior-record path when appropriate.

## Proof

Ran:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' 2>&1 | tee test_after.log`

Result: passed. Build completed, and `ctest` reported 162 backend tests passed,
0 failed. Proof log: `test_after.log`.
