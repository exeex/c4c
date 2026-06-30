Status: Active
Source Idea Path: ideas/open/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Coordinate-Bearing Parallel-Copy Evidence

# Current Packet

## Just Finished

Closed idea 461 as complete diagnostic/probe support and activated idea 462
for the proven RV64 preterminator predecessor-edge parallel-copy residual.

The selected follow-up starts from coordinate-bearing object-route evidence,
not prepared dump order, raw BIR shape, or testcase name:

| Field | Value |
| --- | --- |
| Event kind | `pre_terminator_copies` |
| Function/block | `main`, `block_index=10`, `block_label=logic.rhs.end.40` |
| Move-bundle phase | `block_entry` |
| Parallel copy | `logic.rhs.end.40 -> logic.end.41`, `execution_site=predecessor_terminator` |
| Authority | `out_of_ssa_parallel_copy` |
| Move | `from_value_id=20` to `to_value_id=21`, `destination_storage=register`, `reason=phi_join_register_to_register` |
| Suppression/cast authority | `select_edge_suppression_authorized=no`, `cast_dependency_stack_publication_authorized=no` |
| Fragment status | `generic_move_bundle_materialization_failed` |

## Suggested Next

Execute Step 1 from `plan.md`: audit the coordinate-bearing evidence, prepared
move-bundle/value-home facts, source/destination availability, edge identity,
execution site, and authority facts for the exact `logic.rhs.end.40 ->
logic.end.41` predecessor-edge copy.

Suggested artifact directory:
`build/agent_state/462_step1_preterminator_parallel_copy_audit/`.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not add generic stack-to-register, register-to-register, or all-purpose
  move-bundle support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not reopen ideas 456, 458, 459, 460, or 461 without new coordinate-bearing
  evidence that their exact route owns the first failure.
- Do not infer ownership from prepared dump order, raw BIR shape, value ids
  alone, block labels alone, function names, filenames, or testcase name.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Lifecycle activation proof:

```sh
git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
python3 scripts/plan_review_state.py show
```

Result: passed.
