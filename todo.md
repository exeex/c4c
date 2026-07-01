Status: Active
Source Idea Path: ideas/open/495_prepared_move_bundle_materialization_bucket_review.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce The Move-Bundle Bucket

# Current Packet

## Just Finished

Idea 500 closed after Step 4 published final semantic global/static GEP
admission records from matching available `global_static_gep_authority`
records. Pointer/string, runtime/string intrinsic, aggregate/member, RV64/MIR
lowering, and object emission consumers remain outside idea 500.

Lifecycle switched to idea 495 because the remaining open inventory already
contains the priority `unsupported_move_bundle_target_shape` review/splitter
from the RV64 gcc_torture follow-up plan.

## Suggested Next

Execute Step 1 by reproducing the current move-bundle bucket from
`build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv` and writing a
durable row-count and representative-shape artifact.

## Watchouts

- Do not implement move-bundle lowering in this review idea.
- Do not infer move authority, edge identity, storage class, or phi/join
  semantics in RV64 from raw BIR shape or testcase names.
- Preserve RV64 gcc_torture as external evidence and keep default CTest
  contracts unchanged.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Lifecycle close/switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
