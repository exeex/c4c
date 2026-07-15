# Current Packet

Status: Active
Source Idea Path: ideas/open/793_lir_to_new_bir_remaining_coverage_umbrella.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify remaining families by first owner

## Just Finished

- Step 1 established the current post-792/post-7.30 baseline in
  `docs/lir_to_new_bir_remaining_coverage/post_792_post_730_evidence_baseline.md`.
  It records the accepted 734 Step 7.30 VLA stack-save receipt and keeps all
  nonselected local/VLA and broader families fail closed.

## Suggested Next

- Execute Step 2: classify every remaining family by first owner and dependency
  order from the new post-792/post-7.30 baseline.

## Watchouts

- Closed 792 authorizes only the VLA stack-save handoff. Stack restore, dynamic
  VLA allocation, VLA GEP, other local rows, and broader local conversion stay
  fail closed; do not derive authority from presentation text.

## Proof

- Passed: `git diff --check`; structural check confirmed the Step 1 baseline is
  under the owned handoff directory, names post-792/post-7.30 evidence, and
  changes no code. No build or test applies to this documentation-only packet.
