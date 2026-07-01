# RV64 Before-Instruction Prepared Move Materialization

Status: Open
Type: Focused RV64 materialization idea
Parent: `ideas/open/495_prepared_move_bundle_materialization_bucket_review.md`
Source Evidence:
- `build/agent_state/495_step2_move_bundle_coherence/summary.md`
- `build/agent_state/495_step2_move_bundle_coherence/classification.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv`
Owning Layer: RV64 object lowering for coherent prepared before-instruction moves

## Goal

Materialize coherent prepared before-instruction move bundles in RV64 without
reconstructing move authority, storage identity, or consumer ordering from raw
BIR shape.

## Why This Exists

Idea 495 Step 2 classified 328 `unsupported_move_bundle_target_shape` rows as
coherent prepared-authority before-instruction copies that fail only because
RV64 does not materialize the prepared moves:

- 278 `before_instruction_copies` rows with destination `stack_slot` and move
  reason `consumer_register_to_stack`;
- 50 `before_instruction_copies` rows with destination `stack_slot` and move
  reason `consumer_stack_to_stack`.

Representative rows include `src/20010123-1.c` and `src/20040709-1.c`.

## In Scope

- Consume existing prepared before-instruction move-bundle facts with
  `phase=before_instruction`, `authority=none`, `parallel_copy=no`, and
  destination `stack_slot`.
- Lower register-to-stack and stack-to-stack consumer moves before the consuming
  instruction in RV64.
- Preserve prepared ordering and storage identity exactly as published.
- Add focused backend coverage for representative register-to-stack and
  stack-to-stack rows plus fail-closed boundaries for incomplete prepared facts.

## Out Of Scope

- Out-of-SSA/pre-terminator parallel-copy materialization.
- The single before-return move shape.
- Select-publication evidence or authority repair.
- Producer/prepared fact changes unless a separate producer gap is proven and
  routed to its own idea.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- RV64 materializes coherent before-instruction prepared move bundles from the
  prepared record stream, not from testcase shape.
- Register-to-stack and stack-to-stack representative rows no longer fail for
  `unsupported_move_bundle_target_shape` for this owner family.
- Missing, ambiguous, or contradictory prepared move facts remain fail-closed
  and do not trigger target inference.
- Backend proof covers focused before-instruction move materialization and
  preserves existing backend tests.

## Reviewer Reject Signals

- Reject code that keys behavior on testcase names, source files, function
  names, raw instruction order, or case-log text instead of prepared move
  records.
- Reject RV64 reconstruction of destination storage, source storage, edge
  identity, or consumer ordering when prepared facts are missing.
- Reject combining this owner with out-of-SSA, before-return, select-publication
  evidence, F128, call ABI, or broad stack-frame rewrites.
- Reject expectation, allowlist, unsupported-marker, pass/fail accounting, or
  runtime-comparison changes as proof of progress.
