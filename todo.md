Status: Active
Source Idea Path: ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 for idea 461: recorded residual disposition and close
readiness under `build/agent_state/461_step4_residual_disposition/`.

Idea 461 is close-ready as diagnostic/probe support. Step 3 produced
coordinate-bearing evidence for the first `20010329-1` RV64 move-bundle
failure:

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

Residual owner classification: the proven failure is not the earlier `4:1`
cast-dependency stack-publication candidate, not an idea-459 suppression
matcher failure, and not stale stack-load authority. The follow-up owner should
be a separate RV64 select-result/preterminator predecessor-edge parallel-copy
materialization or route-classification idea for the `logic.rhs.end.40 ->
logic.end.41` out-of-SSA select-result copy.

## Suggested Next

Plan-owner close review for idea 461. If more progress is desired, split or
activate a new source idea for the proven `pre_terminator_copies` /
`out_of_ssa_parallel_copy` residual owner. Do not select semantic lowering in
idea 461.

## Watchouts

- This is diagnostics/probe support, not semantic lowering.
- Do not add generic stack-to-register or register-to-register move support.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- The first proven `20010329-1` failure is not the earlier `4:1` cast stack
  publication candidate and is not an idea-459 suppression matcher failure.
- Do not infer ownership from value ids, block indexes, instruction indexes,
  raw BIR shape, filenames, function names, or one prepared dump alone; use
  the object-route coordinate diagnostic.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 4 proof:

```sh
git diff --check
```

Result: passed.
