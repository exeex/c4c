Status: Active
Source Idea Path: ideas/open/461_rv64_move_bundle_coordinate_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Diagnostic Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 461: implemented diagnostic-only event-site
coordinate formatting for generic RV64 prepared move-bundle fragment failures,
added focused backend coverage, and saved fresh `20010329-1` evidence under
`build/agent_state/461_step3_coordinate_diagnostic_packet/`.

The new diagnostic preserves the old prefix and appends event kind, function,
block index/label, instruction index, move-bundle phase and authority,
parallel-copy context, select-edge suppression and cast-dependency
stack-publication authority match states, per-move source/destination/storage
fields, move reason, and fragment status.

Fresh `20010329-1` object-route evidence now identifies the first proven
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

## Suggested Next

Execute Step 4: residual disposition and close-readiness. Use the coordinate
evidence to decide whether idea 461 is complete and route the next source owner
to the proven `logic.rhs.end.40 -> logic.end.41` preterminator predecessor-edge
select-result parallel-copy failure. Do not select lowering inside idea 461.

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

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
