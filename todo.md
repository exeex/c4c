Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Remaining Edge-Copy Contract Gaps
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Activation reset for idea 90. No executor packet has landed on this runbook
yet.

## Suggested Next

Inspect the current `out_of_ssa` publication, dump, and consumer surfaces to
identify the first missing target-independent edge-copy contract fact for Step
1.

## Watchouts

- Keep this route target-independent; do not repair critical-edge or copy-order
  gaps with x86-local recovery.
- Treat grouped-register behavior as in scope only where it changes published
  edge-copy authority.
- Preserve idea 87's completed ownership of phi elimination while deepening
  the remaining bundle semantics.

## Proof

Not run yet for idea 90 activation.
