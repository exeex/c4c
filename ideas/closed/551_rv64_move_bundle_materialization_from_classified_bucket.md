# RV64 Move-Bundle Materialization From Classified Bucket

Status: Closed
Type: RV64/MIR implementation queue
Parent: `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
Owning Layer: RV64/MIR object lowering

## Goal

Implement RV64/MIR materialization for the 151 current
`unsupported_move_bundle_target_shape` rows classified as
`coherent_rv64_mir_materialization`.

## Why This Exists

The bucket split review found a large implementation-ready lane where current
logs publish concrete move coordinates, value ids, compatible prepared source
and destination homes, scalar types, and the generic RV64 materialization
failure. These rows should be handled as RV64 move-bundle lowering work, not
mixed with prepared authority repair or evidence gathering.

## Evidence Anchor

- Source splitter: `ideas/open/544_rv64_move_bundle_target_shape_bucket_split.md`
- Classification table:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.tsv`
- Classification note:
  `docs/rv64_gcc_torture_post_contract/move_bundle_target_shape_classification.md`
- Lane filter: `first_owner_lane=coherent_rv64_mir_materialization`
- Current row count: 151

Current subqueue counts from the classification table:

- 130 `consumer_register_to_stack/register_to_stack_slot` rows.
- 15 `consumer_register_to_stack/rematerializable_immediate_to_stack_slot`
  rows.
- 3 `phi_join_register_to_register/rematerializable_immediate_to_register`
  rows.
- 2 `consumer_stack_to_stack/stack_slot_to_stack_slot` rows.
- 1 `phi_join_register_to_register/select_publication_immediate_to_register`
  row.

## In Scope

- Add general RV64/MIR lowering for prepared move bundles whose source and
  destination homes are already coherent in the classification evidence.
- Cover register-to-stack, rematerializable-immediate-to-stack,
  stack-to-stack, and phi register-to-register families represented by the
  151-row lane.
- Preserve row-level evidence and re-run the matching gcc_torture subset or
  an equivalent focused proof chosen by the supervisor.
- Keep implementation behavior tied to semantic prepared homes and scalar
  type facts, not testcase names.

## Out Of Scope

- Prepared/module target-shape authority repair.
- BIR semantic producer or admission repair.
- F128 policy, ABI, helper, or quarantine changes.
- Expectation rewrites, unsupported marker edits, allowlist filtering, or
  runtime comparison changes.
- Special casing individual filenames from the 151-row set.

## Acceptance Criteria

- A coherent subset of the 151 classified rows advances because RV64 consumes
  already-published prepared move facts.
- The implemented rule is semantic over move shape and prepared homes, not
  filename or expected assembly spelling.
- Any row that exposes a missing prepared or BIR fact during implementation is
  removed from this lane and routed back to the appropriate producer idea.
- Fresh proof shows no supported-path regression for the selected subset.

## Closure Summary

Closed after the Step 9 residual reconciliation. RV64/MIR materialization work
advanced the coherent register-to-stack, immediate-to-stack, stack-to-stack,
and register publication families without expectation rewrites, unsupported
marker changes, external allowlist changes, runtime comparison changes, or
filename-specific lowering.

Final residual accounting for the 20-row Step 6 tail is preserved in
`docs/rv64_gcc_torture_post_contract/move_bundle_materialization_residual_20_reconciliation.md`:

- 5 rows now pass.
- 12 rows are routed to prepared move-bundle authority repair.
- 2 rows advanced to later runtime mismatch work.
- 1 row advanced to a later explicit unsupported diagnostic.
- 0 rows remain as the same generic move-bundle materialization failure without
  reroute evidence.

The 12 prepared-authority residuals are carried into
`ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`; RV64
must not repair them by inferring destination homes, pointer-base stack slots,
or missing source size/type facts.

## Reviewer Reject Signals

- Reject named-case or filename-specific materialization shortcuts.
- Reject lowering that guesses source or destination homes from testcase names,
  raw BIR fragments, expected target register spelling, or final RV64 shape.
- Reject mixing the 31 prepared authority rows or the `src/960209-1.c`
  evidence-gap row into this implementation route.
- Reject unsupported downgrades, expectation rewrites, allowlist filtering, or
  weaker runtime comparison as progress.
- Reject helper renames or diagnostic text changes claimed as capability
  progress while the same generic move-bundle materialization failure remains.
