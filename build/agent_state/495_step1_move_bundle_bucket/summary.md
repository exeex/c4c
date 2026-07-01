# 495 Step 1 Move-Bundle Bucket Reproduction

## Input Evidence

- Source summary: `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`
- Current scan log pointer:
  `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`
- Current scan log:
  `build/agent_state/rv64_gcc_torture_backend_current_20260630T032216Z.log`
- Case logs: paths from the summary TSV `log` column.

The summary TSV carries `status`, `case`, and `log`; the bucket label is in the
referenced case logs. Step 1 joined the TSV rows to case logs containing
`unsupported_move_bundle_target_shape`.

## Row Count

- Summary rows scanned: 1,467 data rows.
- Summary fail rows: 1,153.
- Move-bundle bucket rows reproduced: 423.
- Source cases: 423, listed in `source_cases.txt`.
- Function/block evidence available: 420 rows.
- Rows without function/block evidence: 3 select-publication move-bundle rows.

## Durable Artifacts

- `rows.tsv`: joined case/log/bucket/fragment evidence.
- `parsed_rows.tsv`: tokenized rows with event kind, function, block label,
  instruction index, phase, authority, move count, parallel-copy flag,
  destination storage, first move reason, and fragment status.
- `source_cases.txt`: source case list.
- `shape_counts.txt`: raw bucket-shape counts.
- `fragment_status_counts.txt`: fragment status counts.
- `function_block_counts.txt`: grouped function/block occurrences where
  available.
- `shape_matrix.tsv`: grouped representative shape matrix.
- `representative_rows.tsv`: one representative row per matrix shape.

## Shape Counts

```text
420 prepared module shape: unsupported_move_bundle_target_shape: prepared move
  3 prepared module shape: unsupported_move_bundle_target_shape: prepared
```

The 420 ordinary prepared-move rows all report
`fragment_status=generic_move_bundle_materialization_failed`. The 3
select-publication rows have no `fragment_status=` token in the case log.

## Representative Shape Matrix

```text
278 unsupported_move_bundle_target_shape: prepared before_instruction_copies before_instruction none no stack_slot consumer_register_to_stack
 62 unsupported_move_bundle_target_shape: prepared pre_terminator_copies block_entry out_of_ssa_parallel_copy yes register phi_join_register_to_register
 50 unsupported_move_bundle_target_shape: prepared before_instruction_copies before_instruction none no stack_slot consumer_stack_to_stack
 26 unsupported_move_bundle_target_shape: prepared pre_terminator_copies block_entry out_of_ssa_parallel_copy yes register edge_consumer_preservation_register_to_register
  3 unsupported_move_bundle_target_shape: prepared pre_terminator_copies block_entry out_of_ssa_parallel_copy yes stack_slot edge_consumer_preservation_register_to_stack
  3 unsupported_move_bundle_target_shape: prepared <select-publication shape, no per-function tokens>
  1 unsupported_move_bundle_target_shape: prepared pre_terminator_copies before_return none no register return_stack_to_register
```

Representative rows are in `representative_rows.tsv`:

- `src/20010123-1.c`: before-instruction consumer register-to-stack move.
- `src/20040709-1.c`: before-instruction consumer stack-to-stack move.
- `src/20020206-2.c`: block-entry out-of-SSA parallel-copy edge preservation,
  register destination.
- `src/20020619-1.c`: block-entry out-of-SSA phi join, register destination.
- `src/20041114-1.c`: block-entry out-of-SSA edge preservation,
  stack-slot destination.
- `src/20080719-1.c`: before-return return stack-to-register move.
- `src/builtin-constant.c`: select-publication move bundle with no
  function/block tokens.

## Step 2 Handoff

Step 2 should classify these rows into first-owner packets. The evidence
strongly separates at least three families:

- consumer move materialization before ordinary instructions
- out-of-SSA/pre-terminator parallel-copy materialization
- select-publication move bundles without per-function case-log tokens

No implementation changes were made in Step 1.

## Proof

Delegated proof command:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
