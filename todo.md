Status: Active
Source Idea Path: ideas/open/05_prepared_call_argument_source_selection_completeness.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Update Printing/Diagnostics And Add Focused Coverage

# Current Packet

## Just Finished

Completed Step 3 for `plan.md`: updated prepared call-plan printing so call
arguments with structured source selections print `arg.source_selection` plus
the source identity, selected prior call position, preservation route, and
callee-saved register or stack-slot payload facts.

Added focused backend prepared-printer coverage for a real callee-saved
prior-preservation argument source selection and for stack-slot
prior-preservation selection payload printing via a manual prepared-printer
fixture.

## Suggested Next

Execute the next Step 4 packet: supervisor should decide whether this Step 3
observability slice is sufficient to advance to final validation, review, or
plan closure work.

## Watchouts

The natural x86 stack-preservation fixture preserves through a stack slot, but
its later argument-source path selects frame-slot value facts before the
prior-preservation path; stack-slot prior-preservation printing is therefore
covered with a manual prepared-printer fixture rather than claiming planner
selection behavior that is not present.

## Proof

Ran:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' 2>&1 | tee test_after.log`

Result: passed. Build completed, and `ctest` reported 162 backend tests passed,
0 failed. Proof log: `test_after.log`.
