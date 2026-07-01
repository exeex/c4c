Status: Active
Source Idea Path: ideas/open/495_prepared_move_bundle_materialization_bucket_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Prepared Bundle Coherence

# Current Packet

## Just Finished

Step 2 classified the 423 reproduced `unsupported_move_bundle_target_shape`
rows from Step 1 by prepared event kind, authority, parallel-copy status,
execution site, destination storage, source storage where reported, move
reason, coherence family, and first owner. Durable evidence is recorded under
`build/agent_state/495_step2_move_bundle_coherence/`: `classification.tsv`,
`coherence_counts.tsv`, `owner_matrix.tsv`,
`representative_classification.tsv`, and `summary.md`.

The classification keeps the Step 1 row count intact. It separates 420 rows
with coherent prepared authority but missing RV64 materialization support from
3 select-publication move-bundle rows whose current case-log tokens omit
function/block/event/phase/authority/storage details. There are no contradictory
prepared-authority rows in the reproduced bucket.

## Suggested Next

Execute Step 3 follow-up splitting. Split implementation/evidence ownership into
focused packets for RV64 before-instruction prepared move materialization, RV64
out-of-SSA/pre-terminator parallel-copy materialization, the single before-return
move materialization shape, and select-publication move-bundle evidence/authority
before treating those 3 rows as lowerable.

## Watchouts

- This packet made no implementation changes.
- The summary TSV itself does not contain the bucket label; the label is in the
  referenced case logs.
- Do not treat source names as authority. Use the parsed move-bundle event
  tokens, authority, phase, destination storage, parallel-copy flag, execution
  site, and move reason for Step 3 splitting.
- Source storage is not reported by the current case-log token stream for these
  representatives; do not infer it from the source case name.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Step 2 evidence-only proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
