Status: Active
Source Idea Path: ideas/open/05_prepared_call_argument_source_selection_completeness.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Update Emission, Printing, And Diagnostics

# Current Packet

## Just Finished

Completed Step 3 for `plan.md`: updated AArch64 call-argument emission so the
selection overload of `make_prior_preserved_call_argument_source` consumes
complete explicit callee-saved `PriorPreservation` selection facts directly.

The overload now constructs the preserved source register from the selected
register name, bank, contiguous width, occupied register names, and register
placement without consulting `PreparedCallPreservedValue`. Stack-slot selection
behavior remains fail-closed on the existing complete stack fact set, and
`lower_before_call_move` no longer falls back to preserved-record lookup when a
complete explicit callee-saved prior-preservation selection fails emission.

## Suggested Next

Execute the next Step 3/4 packet by adding natural printer or diagnostic
coverage for explicit prior-preservation source-selection facts if the
supervisor wants surfaced observability beyond the current emission behavior.

## Watchouts

Legacy preserved-record fallback still exists for paths without a complete
explicit callee-saved `PriorPreservation` selection. This packet did not edit
shared prepared-record files or tests.

## Proof

Ran:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' 2>&1 | tee test_after.log`

Result: passed. Build completed, and `ctest` reported 162 backend tests passed,
0 failed. Proof log: `test_after.log`.
