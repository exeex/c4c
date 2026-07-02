Status: Active
Source Idea Path: ideas/open/548_prepared_global_stack_frame_infrastructure_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Infrastructure Bucket Evidence

# Current Packet

## Just Finished

Activated plan Step 1 from
`ideas/open/548_prepared_global_stack_frame_infrastructure_review.md`.

## Suggested Next

Execute Step 1 by reconstructing current representative evidence for
`unsupported_global_data`, `unsupported_stack_frame`, and
`unsupported_prepared_move_bundle_classification`, then record rows, log paths,
counts or traceable evidence source, and first-owner hints in this file.

## Watchouts

- This is a review/classification plan. Do not implement prepared or RV64
  lowering inside this active idea.
- Do not treat downstream `unsupported_global_data` rows from recently closed
  producer ideas as implementation-ready until prepared versus RV64 ownership
  is proven.
- Route primary-F128 rows to the F128 quarantine lane.
- Do not change expectations, unsupported markers, allowlists, or pass/fail
  accounting as evidence of progress.

## Proof

Initial activation only. No build proof required.
