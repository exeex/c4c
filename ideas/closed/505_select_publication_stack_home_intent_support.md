# Select-Publication Stack-Home Intent Support

Status: Closed
Type: Focused prepared/RV64 intent-support idea
Parent: `ideas/closed/504_select_publication_move_bundle_evidence_authority.md`
Source Evidence:
- `build/agent_state/504_step3_select_publication_consumer_classification/summary.md`
- `build/agent_state/504_step3_select_publication_consumer_classification/rows.tsv`
- `build/agent_state/504_step3_select_publication_consumer_classification/classification_counts.tsv`
Owning Layer: EdgePublicationMoveIntent support before RV64 select-publication materialization

## Goal

Teach the select-publication intent layer to represent stack-source and
stack-destination publication moves explicitly, or preserve distinguishable
fail-closed statuses when the prepared facts are insufficient.

## Why This Exists

Idea 504 published structured evidence for the formerly ambiguous
select-publication move-bundle rows. Step 3 found two real select-publication
evidence rows that are not final RV64 consumer-ready:

- `src/builtin-constant.c`: `phi_join_stack_to_register` with stack-slot source,
  register destination, and `intent_status_unsupported_source_home`.
- `src/pr58726.c`: `phi_join_stack_to_stack` with register source,
  stack-slot destination, and `intent_status_unsupported_destination_home`.

These rows are not ready for RV64 materialization because
`EdgePublicationMoveIntent` does not expose supported concrete stack source or
stack destination intent for these select-publication shapes.

## In Scope

- Audit `EdgePublicationMoveIntent` construction for select-publication
  stack-source and stack-destination moves.
- Publish supported intent only when prepared source/destination homes, stack
  layout, value identity, edge identity, carrier, and move-bundle facts are
  explicit and coherent.
- Preserve fail-closed statuses for missing, ambiguous, contradictory, or
  raw-shape-only stack-home intent.
- Decide whether a later RV64 select-publication materialization consumer idea
  is warranted after intent support is explicit.

## Out Of Scope

- Final RV64 materialization of select-publication move bundles.
- Generic out-of-SSA immediate materialization for `src/pr37924.c`.
- Before-instruction, out-of-SSA register/stack preservation, or before-return
  materialization already handled by ideas 501-503.
- Inferring source or destination homes from testcase names, raw BIR shape,
  object output, final registers, or case-log absence.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- The two select-publication evidence rows either publish supported
  stack-source/stack-destination intent or retain precise unsupported statuses.
- Supported intent requires explicit prepared homes and stack layout; raw shape
  is never enough.
- No RV64 select-publication consumer is implemented before intent support is
  available.
- Focused backend proof covers available and fail-closed intent statuses.

## Reviewer Reject Signals

- Reject RV64 materialization added before the intent layer exposes explicit
  supported stack-home intent.
- Reject deriving stack source/destination authority from testcase names,
  source shape, final homes, object output, or target registers.
- Reject folding generic `phi_join_immediate_materialization` into this
  select-publication intent idea.
- Reject expectation, allowlist, unsupported-marker, pass/fail accounting, or
  runtime-comparison changes as proof of progress.

## Completion

Closed after residual disposition. The focused stack-home intent-support scope
is complete:

- Step 1 inspected `EdgePublicationMoveIntent` construction for the two
  select-publication stack-home rows.
- Step 2 published supported intent fields for pointer/XLEN stack-source to
  GPR destination and direct GPR source to 1/2/4-byte stack destination
  select-publication shapes.
- Final RV64 select-publication admission remains fail-closed by design.

The final RV64 select-publication consumer is split into
`ideas/open/507_rv64_select_publication_stack_home_materialization.md`.
Generic out-of-SSA `phi_join_immediate_materialization` remains separate in
`ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md`.

The rejected `test_baseline.new.log` remains preserved for
`string_authority_guard` diagnosis and is not part of this lifecycle closure.
