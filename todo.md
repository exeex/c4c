Status: Active
Source Idea Path: ideas/open/501_rv64_before_instruction_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Before-Instruction Prepared Move Surfaces

# Current Packet

## Just Finished

Idea 495 closed as a completed move-bundle review/splitter. It reproduced the
423-row bucket, classified prepared-authority coherence, and created focused
follow-up ideas 501-504.

Lifecycle activated idea 501 because the supervisor selected it first and it
owns the largest coherent prepared-authority family: 328 before-instruction
rows split between `consumer_register_to_stack` and `consumer_stack_to_stack`.

## Suggested Next

Execute Step 1 by inspecting the prepared before-instruction move-bundle
publication and RV64 consumption surfaces. Record the exact records/helpers to
consume, representative proof rows, and whether the current prepared facts are
complete enough for implementation.

## Watchouts

- Do not implement out-of-SSA, before-return, or select-publication move
  handling under idea 501.
- Do not infer move authority, destination storage, source storage, or consumer
  ordering from testcase names, raw BIR shape, case-log text, final homes, or
  object output.
- If producer facts are incomplete, route the producer gap instead of
  reconstructing it in RV64.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Lifecycle close/switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
