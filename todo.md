Status: Active
Source Idea Path: ideas/open/04_aarch64_prior_preservation_baseline_drift.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Inspect

# Current Packet

## Just Finished

Lifecycle activation for the AArch64 prior-preservation baseline drift repair.

## Suggested Next

Inspect `calls_moves.cpp` around the `11b33c8d0586` change and reproduce one
of the 37 failing AArch64 c_testsuite cases.

## Watchouts

Do not restore broad prior-home rederivation for incomplete explicit
selections. The fix must distinguish complete prior-preservation selections
from unrelated source-selection kinds.

## Proof

Not run yet.
