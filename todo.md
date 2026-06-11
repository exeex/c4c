Status: Active
Source Idea Path: ideas/open/173_route7_comparison_condition_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory The Current Consumer Boundary

# Current Packet

## Just Finished

Activation created the runbook for idea 173. No implementation packet has run
yet.

## Suggested Next

Delegate Step 1 to inspect the current Route 7 and AArch64 comparison consumer
boundary, then update this file with the selected consumer and proof subset for
Step 2.

## Watchouts

- Keep target branch spelling, fused-compare legality, condition-code
  selection, hazards, emitted-register state, and final instruction records out
  of BIR.
- Do not contract prepared comparison helpers before the selected consumer has
  Route 7-backed equivalence proof.
- Treat helper renames, expectation rewrites, or named-case-only fixes as
  non-progress.

## Proof

Lifecycle activation only; no build or test proof required.
