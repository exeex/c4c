Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Starting-State Value-Flow Boundary

# Current Packet

## Just Finished

Lifecycle split completed: idea 359 was parked as complete for the
stack-preserved pointer-formal post-call overwrite scope, and idea 360 was
activated for the remaining `00181` starting-state output mismatch.

## Suggested Next

Execute plan Step 1. Localize where `00181` first loses the source-tower value
that should print as `3` in the initial state: compare source, semantic BIR,
prepared BIR, generated AArch64, and the first print path before editing.

## Watchouts

- Do not reopen idea 359's stack-preserved pointer formal post-call repair
  unless fresh first-bad-fact evidence points back there.
- The known visible mismatch is expected `A: 1 2 3 4`, actual `A: 1 2 0 4`.
  It appears before recursive post-call `%p.spare` reuse is observable.
- Preserve idea 357, 358, and 359 coverage while localizing the new owner.

## Proof

No Step 1 execution proof yet after activation.
