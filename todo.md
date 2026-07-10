Status: Active
Source Idea Path: ideas/open/675_post_wave_residual_baseline_failures.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconcile The Post-676/677 Residual Set

# Current Packet

## Just Finished

Step 1 completed: reconciled the post-676/677 residual set by stable test name
and preserved the evidence in
`build/agent_state/675_post_676_677_residual_reconciliation/summary.md`.
Fresh backend proof leaves only
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
failing in backend scope. The 676 and 677 rows are settled by their closure
notes and are not current backend proof failures. Current row-322 evidence
shows a dump-contract mismatch: the test still expects `source_value_id=2732`
for `arg index=12`, while current prepared-BIR emits `source_value_id=2728`;
closed idea 665 records `2732` as the rejected later-call lookahead route.

## Suggested Next

Proceed to Step 2 classification for
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`:
decide whether to route row 322 as prepared-BIR dump-contract / expectation
review over valid current-call facts, or require a focused probe that proves a
different current-call aggregate-carrier authority without using the rejected
later-call lookahead.

## Watchouts

- Current `test_baseline.new.log` reports `5/3397` failures and still lists the
  two 676/677 stable names, but it remains diagnostic only and was not accepted
  or modified.
- Do not treat row-count improvement or numeric row id movement as baseline
  acceptance.
- Do not repair row 322 through a later-call/later-store lookahead. That route
  was already rejected as temporal route drift in
  `review/row322_later_lane_review.md`.
- Closed idea 668 keeps the two LLVM torture rows separate from RV64,
  prepared CLI, AArch64, and object-emission ownership absent focused evidence.

## Proof

Ran
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication$|^backend_'`
with output preserved in `test_after.log`. Build passed; CTest failed
nonzero with `1/368` backend failures, exactly
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.
Also captured the current AArch64 prepared-BIR dump for `00204.c` under
`build/agent_state/675_post_676_677_residual_reconciliation/`.
