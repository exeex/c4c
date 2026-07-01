# RV64 Before-Return Prepared Move Materialization

Status: Open
Type: Focused RV64 materialization idea
Parent: `ideas/open/495_prepared_move_bundle_materialization_bucket_review.md`
Source Evidence:
- `build/agent_state/495_step2_move_bundle_coherence/summary.md`
- `build/agent_state/495_step2_move_bundle_coherence/classification.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv`
Owning Layer: RV64 object lowering for coherent prepared before-return moves

## Goal

Handle the isolated coherent before-return prepared move shape without folding
return-specific semantics into broader before-instruction or out-of-SSA
materialization.

## Why This Exists

Idea 495 Step 2 found one coherent prepared-authority row:

- `pre_terminator_copies` with `phase=before_return`, `authority=none`,
  `parallel_copy=no`, destination `register`, and move reason
  `return_stack_to_register`.

The representative row is `src/20080719-1.c`. Its execution point is
return-adjacent and should not be silently absorbed into unrelated move-bundle
owners unless implementation later proves the same lowering helper covers it
without weakening the ownership boundary.

## In Scope

- Consume prepared before-return move facts for stack-to-return-register
  materialization.
- Preserve the return-adjacent execution point and return value storage
  semantics.
- Decide during implementation whether this can share internal helpers with
  other move materializers while keeping this owner family tested separately.
- Add focused backend coverage for the representative before-return row and
  fail-closed incomplete-authority boundaries.

## Out Of Scope

- General return lowering, call ABI rewrites, or stack-frame redesign.
- Before-instruction consumer moves.
- Out-of-SSA/pre-terminator parallel-copy moves.
- Select-publication evidence or authority repair.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- RV64 materializes the coherent before-return prepared move from explicit
  prepared facts.
- The representative before-return row no longer fails for
  `unsupported_move_bundle_target_shape` for this owner family.
- Missing return move authority or ambiguous return storage remains
  fail-closed.
- Backend proof covers the isolated before-return path without regressing the
  broader backend suite.

## Reviewer Reject Signals

- Reject broad return/ABI rewrites that are not required for this single owner
  family.
- Reject inferring return storage from testcase shape, source names, raw BIR
  order, or target register conventions without prepared facts.
- Reject merging this idea into before-instruction or out-of-SSA work without
  preserving a separate representative test/proof for `before_return`.
- Reject expectation, allowlist, unsupported-marker, pass/fail accounting, or
  runtime-comparison changes as progress.
