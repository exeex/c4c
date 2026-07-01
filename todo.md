Status: Active
Source Idea Path: ideas/open/503_rv64_before_return_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Before-Return Move Surfaces

# Current Packet

## Just Finished

Idea 502 closed after Step 4 residual disposition. The 91 coherent
out-of-SSA/pre-terminator parallel-copy rows from idea 495 are now covered by
focused RV64 object-emission paths:

- 62 `phi_join_register_to_register` rows.
- 26 `edge_consumer_preservation_register_to_register` rows.
- 3 `edge_consumer_preservation_register_to_stack` rows.

Remaining move-bundle work stays outside idea 502. The before-return shape is
owned by idea 503, and select-publication evidence/authority is owned by idea
504. Any downstream RV64 gcc-torture admission reclassification or broader
bucket refresh should be handled as a separate lifecycle initiative if needed.

## Suggested Next

Execute Step 1 by inspecting the prepared before-return move-bundle publication
and RV64 consumption surfaces for the `return_stack_to_register` shape. Record
the exact records/helpers to consume, the representative proof row, and whether
the current prepared facts are complete enough for implementation.

## Watchouts

- Do not re-open before-instruction or out-of-SSA parallel-copy materialization
  under idea 503.
- Do not implement select-publication evidence or authority under idea 503.
- Do not infer return storage from testcase shape, source names, raw BIR order,
  final homes, target register conventions, or object output.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Lifecycle close/switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
