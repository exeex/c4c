# RV64 Preterminator Predecessor-Edge Parallel-Copy Materialization

Status: Open
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

## Acceptance Criteria

- The exact `pre_terminator_copies` / `out_of_ssa_parallel_copy` event is
  classified with either an accepted RV64 materialization contract or a precise
  blocker routed to a separate source idea.
- Any implementation consumes explicit coordinate/authority-backed prepared
  facts and remains fail-closed for raw-only, missing-authority, mismatched
  edge, non-register destination, stale stack-load, and generic move cases.
- Focused coverage proves the selected packet and adjacent fail-closed
  boundaries.
- Fresh residual disposition records whether `20010329-1` advanced and routes
  any next first-owner without broadening this idea.

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
