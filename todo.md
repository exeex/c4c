Status: Active
Source Idea Path: ideas/open/08_bir-address-projection-model-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Final Behavior-Preservation Proof

Code Review Reminder Handled: review/address-projection-step3-review.md found Step 3 helper reuse aligned and recommended continuing Step 3.
Test Baseline Reminder Handled: accepted full-suite test_baseline.log for commit f294c3a4 after 3071 passed, 0 failed.

# Current Packet

## Just Finished

Step 5 final behavior-preservation proof completed for the active address
projection consolidation runbook.

Final proof context:

- Step 3 helper reuse was reviewed in
  `review/address-projection-step3-review.md` and found aligned, with no
  testcase-overfit or expectation rewrite.
- Step 4 naming normalization was committed as `f294c3a4`, and the accepted
  full-suite baseline for that commit reported 3071 passed, 0 failed.
- Final backend proof rebuilt the relevant backend targets and reran the
  `^backend_` subset with no regression.

## Suggested Next

Have the plan owner decide whether the active source idea is complete and
should close, or whether any follow-up Step 5 documentation/proof note is still
needed before closure.

## Watchouts

- No expectation rewrites were made.
- No new headers were added.
- Stateful lowering remains owned by `BirFunctionLowerer`; shared helpers only
  report projection facts.

## Proof

Ran final proof command and wrote combined stdout/stderr to
`test_after.log`:

`cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. CTest reported 100% tests passed, 0 failed out of 97 run;
12 matching backend tests were disabled and not run.

Regression guard compared `test_before.log` and `test_after.log` with
`--allow-non-decreasing-passed`: passed with 97 before, 97 after, 0 new
failures.
