Status: Active
Source Idea Path: ideas/open/05_prepared_call_argument_source_selection_completeness.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Complete Prior-Preservation Selection Facts

# Current Packet

## Just Finished

Completed Step 2 for `plan.md`: extended
`PreparedCallArgumentSourceSelection` so `PriorPreservation` selections can
carry callee-saved register source facts copied from
`PreparedCallPreservedValue`.

The new selection payload carries the preserved register name, register bank,
contiguous width, occupied register names, and typed register placement.
`select_prepared_call_argument_source` now populates those fields for prior
preservation selections and refuses to publish a callee-saved-register
selection when any required register fact is missing. The prior-call coordinate
lookup also remains fail-closed if the selected preserved-value pointer cannot
be matched back to its call record.

## Suggested Next

Execute the next packet by teaching the emission-side selection overload to use
the callee-saved register facts now carried by explicit `PriorPreservation`
source selections, while keeping legacy preserved-record fallback behavior only
for paths that still lack an explicit complete selection.

## Watchouts

This packet intentionally did not edit AArch64 emission. The current callee-
saved path can still fall back through
`find_prior_preserved_value_for_call_argument`; the next slice should consume
the new selection fields instead of adding target-local rederivation.

## Proof

Ran:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' 2>&1 | tee test_after.log`

Result: passed. Build completed, and `ctest` reported 162 backend tests passed,
0 failed. Proof log: `test_after.log`.
