# RV64 Preterminator Predecessor-Edge Parallel-Copy Materialization

Status: Closed
Type: RV64 prepared move-bundle materialization / route-classification idea
Parent: `ideas/closed/461_rv64_move_bundle_coordinate_diagnostics.md`
Source Evidence: `build/agent_state/461_step4_residual_disposition/`
Owning Layer: RV64 preterminator predecessor-edge out-of-SSA parallel-copy materialization

## Goal

Classify and, if justified, implement the RV64 object-route handling for the
proven `20010329-1` first failing move-bundle event: `pre_terminator_copies`
in `main`, `block_index=10`, `block_label=logic.rhs.end.40`,
`phase=block_entry`, `authority=out_of_ssa_parallel_copy`, parallel copy
`logic.rhs.end.40 -> logic.end.41`, `move[0].from_value_id=20` to
`move[0].to_value_id=21`, `destination_storage=register`, and
`reason=phi_join_register_to_register`.

## Why This Exists

Idea 461 completed the diagnostic/probe prerequisite and produced
coordinate-bearing object-route evidence for the first remaining
`unsupported_move_bundle_target_shape` owner. That evidence rules out the
earlier cast-dependency stack publication candidate, the idea-459 suppression
matcher target, stale stack-load authority, and generic unnamed move support.
The next route must use the coordinate-bearing object-route event identity,
not prepared dump order, testcase name, or raw BIR shape.

## In Scope

- Audit the coordinate-bearing Step 461 evidence and the corresponding
  prepared move-bundle/value-home facts for the exact
  `logic.rhs.end.40 -> logic.end.41` predecessor-edge copy.
- Classify whether the event is a sound RV64 register-to-register
  preterminator predecessor-edge parallel copy, a select-result publication
  dependency that must be rematerialized differently, or a producer/prepared
  metadata blocker.
- If a bounded consumer packet is justified, materialize only the proven
  coordinate/authority-backed `out_of_ssa_parallel_copy` register-destination
  case and add focused fail-closed coverage for adjacent raw, missing,
  mismatched, non-register, or non-authorized cases.
- Preserve any remaining unrelated move-bundle, storage, stale-stack-load, or
  select/publication residuals as separate lifecycle owners.

## Out Of Scope

- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Reopening ideas 456, 458, 459, 460, or 461 without new coordinate-bearing
  evidence that their exact route owns the first failure.
- Ownership inference from prepared dump order, raw BIR shape, value ids
  alone, block labels alone, function names, filenames, or testcase name.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.

## Completion Notes

Closed by lifecycle split after Step 4 residual disposition. Idea 462 is
complete as a negative route-classification slice: the coordinate-bearing
`pre_terminator_copies` / `out_of_ssa_parallel_copy` event is not a sound
plain predecessor-edge register copy.

Step 3 retained the same object-route failure:

```text
event_kind=pre_terminator_copies function=main block_index=10 block_label=logic.rhs.end.40 instruction_index=0 phase=block_entry authority=out_of_ssa_parallel_copy parallel_copy_predecessor=logic.rhs.end.40 parallel_copy_successor=logic.end.41 move[0].from_value_id=20 move[0].to_value_id=21 destination_storage=register reason=phi_join_register_to_register fragment_status=generic_move_bundle_materialization_failed
```

The plain-copy route is rejected because `%t46` is produced in successor block
`logic.end.41`, after predecessor block `logic.rhs.end.40` branches there:

```text
logic.rhs.end.40:
  bir.br logic.end.41
logic.end.41:
  %t46 = bir.ule ptr %t42, %t45
  %t50.phi.sel0 = bir.select ne i32 %t36, 0, i32 %t46, 0
```

Prepared GPR homes for `%t46` and `%t50` do not prove `%t46` is available at
the predecessor terminator. A plain `%t46 -> %t50` copy or same-register no-op
would infer predecessor availability from value-home metadata alone.

Follow-up source idea:
`ideas/open/463_select_edge_ule_source_producer_rematerialization.md`.

That follow-up owns why the existing source-producer consumer does not
rematerialize `%t46 = bir.ule ptr %t42, %t45`, how duplicate carrier facts
`%t50.phi.sel0` / `%t50.phi.sel1` should be classified, and whether `%t42` /
`%t45` are target-consumable at predecessor edge `logic.rhs.end.40 ->
logic.end.41`.

Close validation used existing canonical regression logs and `git diff --check`;
no implementation, test, review, or baseline-log files were changed by this
lifecycle transition.

## Reviewer Reject Signals

- Reject changes that add generic move-bundle support while claiming to solve
  only the proven preterminator predecessor-edge copy.
- Reject testcase-shaped handling keyed on `20010329-1`, `main`,
  `logic.rhs.end.40`, value ids, or block indexes without consuming the
  prepared coordinate/authority contract.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn as progress.
- Reject consuming stale stack-load authority or reviving the earlier
  cast-dependency/suppression routes without new coordinate evidence.
- Reject classification-only changes presented as materialization progress
  unless the lifecycle state explicitly records a blocker and routes the next
  owner.
