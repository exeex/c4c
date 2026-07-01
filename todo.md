Status: Active
Source Idea Path: ideas/open/495_prepared_move_bundle_materialization_bucket_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce The Move-Bundle Bucket

# Current Packet

## Just Finished

Step 1 reproduced the current `unsupported_move_bundle_target_shape` bucket by
joining `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv` rows to
their referenced case logs and filtering for the bucket label. The reproduced
bucket has 423 source-case rows out of 1,467 summary data rows; 420 rows carry
ordinary prepared-move diagnostics with function/block evidence, and 3 rows are
select-publication move-bundle diagnostics without per-function tokens in the
case log.

Durable evidence is recorded under
`build/agent_state/495_step1_move_bundle_bucket/`: `rows.tsv`,
`parsed_rows.tsv`, `source_cases.txt`, `shape_counts.txt`,
`fragment_status_counts.txt`, `function_block_counts.txt`,
`shape_matrix.tsv`, `representative_rows.tsv`, and `summary.md`.

## Suggested Next

Execute Step 2 classification over the reproduced bucket. Classify first-owner
families for ordinary before-instruction consumer move materialization,
out-of-SSA/pre-terminator parallel-copy materialization, the single
before-return return move, and the 3 select-publication move-bundle rows.

## Watchouts

- This packet made no implementation changes.
- The summary TSV itself does not contain the bucket label; the label is in the
  referenced case logs.
- Do not treat source names as authority. Use the parsed move-bundle event
  tokens, authority, phase, destination storage, parallel-copy flag, and move
  reason for Step 2 classification.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 1 evidence-only proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
