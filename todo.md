Status: Active
Source Idea Path: ideas/open/502_rv64_out_of_ssa_parallel_copy_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Out-Of-SSA Parallel-Copy Surfaces

# Current Packet

## Just Finished

Idea 501 closed after Step 4 residual disposition. The two coherent
before-instruction destination stack-slot families from idea 495 are now
implemented:

- `consumer_register_to_stack` materialization landed in Step 2.
- `consumer_stack_to_stack` materialization landed in Step 3.

Remaining move-bundle families stay outside idea 501 and remain queued under
ideas 502, 503, and 504.

Lifecycle activated idea 502 because it is the next largest coherent prepared
move-bundle family: 91 out-of-SSA/pre-terminator parallel-copy rows.

## Suggested Next

Execute Step 1 by inspecting the prepared out-of-SSA parallel-copy publication
and RV64 consumption surfaces. Record the exact records/helpers to consume,
representative proof rows, and whether the current prepared facts are complete
enough for implementation.

## Watchouts

- Do not re-open before-instruction moves under idea 502.
- Do not implement before-return or select-publication handling under idea 502.
- Do not infer predecessor/successor, phi, edge, execution-site, destination
  storage, or parallel-copy ordering from raw block order, source shape,
  filenames, labels, final homes, or target output.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Lifecycle close/switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
