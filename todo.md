Status: Active
Source Idea Path: ideas/open/495_prepared_move_bundle_materialization_bucket_review.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Lifecycle Disposition

# Current Packet

## Just Finished

Step 3 split the Step 2 move-bundle owner families into focused open follow-up
ideas:

- `ideas/open/501_rv64_before_instruction_prepared_move_materialization.md`
  for 328 coherent prepared before-instruction rows.
- `ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md`
  for 91 coherent out-of-SSA/pre-terminator parallel-copy rows.
- `ideas/open/503_rv64_before_return_prepared_move_materialization.md`
  for the single coherent before-return row.
- `ideas/open/504_select_publication_move_bundle_evidence_authority.md`
  for 3 select-publication rows that need explicit evidence/authority before
  RV64 materialization.

The split keeps producer/evidence ownership separate from RV64 materialization
and does not implement lowering.

## Suggested Next

Ask the plan owner to execute Step 4 residual disposition for idea 495. The
likely disposition is to close idea 495 as a completed review/splitter and
activate the highest-priority follow-up selected by the supervisor.

## Watchouts

- This packet made no implementation changes.
- Do not activate more than one follow-up idea at a time.
- The select-publication rows remain producer/evidence work until explicit
  event/phase/authority/storage facts are published.
- Do not treat source names as authority. Follow-up execution must consume the
  parsed move-bundle event tokens, authority, phase, destination storage,
  parallel-copy flag, execution site, and move reason.
- Source storage is not reported by the current case-log token stream for these
  representatives; do not infer it from the source case name.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 3 lifecycle-only proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
