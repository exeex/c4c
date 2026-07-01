# RV64 Select-Publication Stack-Home Materialization

Status: Closed
Type: Focused RV64 materialization idea
Parent: `ideas/closed/505_select_publication_stack_home_intent_support.md`
Source Evidence:
- `build/agent_state/505_step2_stack_home_intent_support/summary.md`
- `build/agent_state/505_step1_stack_home_intent_construction/rows.tsv`
- `build/agent_state/504_step3_select_publication_consumer_classification/rows.tsv`
Owning Layer: RV64 object lowering for supported select-publication stack-home intent

## Completion Summary

Closed after the supported RV64 select-publication stack-home consumer paths
landed in commits `1d57dc946` and `f558955d6`.

- Pointer/XLEN concrete stack-source to GPR destination intent now materializes
  through structured `EdgePublicationMoveIntent` authority.
- Direct GPR source to concrete 1/2/4-byte stack-destination intent now
  materializes through structured `EdgePublicationMoveIntent` authority.
- Unsupported widths, large offsets, stack-to-stack select-publication without
  scratch/interleaving policy, missing concrete stack fields, coordinate
  confusion, and generic immediate materialization remain outside this idea.
- Close gate used the lifecycle-only regression guard against the existing
  backend `test_before.log` baseline and passed 328/328 with no regressions.

## Goal

Materialize supported select-publication stack-home intent in RV64 while
preserving fail-closed behavior for unsupported stack widths, offsets,
stack-to-stack shapes, and missing prepared authority.

## Why This Exists

Idea 505 added structured stack-home intent support for the two real
select-publication evidence rows from idea 504, but intentionally left final
RV64 select-publication object admission fail-closed:

- `src/builtin-constant.c`: pointer/XLEN concrete stack source to GPR
  destination intent with `ld` instruction text.
- `src/pr58726.c`: direct GPR source to concrete 1/2/4-byte stack destination
  intent with size-specific store instruction text.

These shapes now have explicit intent fields, so final RV64 materialization can
be considered as a separate consumer packet.

## In Scope

- Consume supported `EdgePublicationMoveIntent` stack-home fields from idea
  505.
- Materialize pointer/XLEN stack-source to GPR destination
  select-publication moves when prepared authority is explicit.
- Materialize direct GPR source to 1/2/4-byte stack-destination
  select-publication moves when prepared authority is explicit.
- Preserve fail-closed behavior for missing edge/destination identity, missing
  concrete stack offset/size, non-GPR destination placement, unsupported stack
  widths, large offsets, and stack-to-stack select-publication without a
  scratch/interleaving policy.

## Out Of Scope

- Generic out-of-SSA `phi_join_immediate_materialization`, owned by idea 506.
- Extending `EdgePublicationMoveIntent` beyond the supported fields already
  published by idea 505 unless a separate producer gap is proven.
- Stack-to-stack select-publication materialization without an explicit scratch
  and interleaving policy.
- Reopening before-instruction, out-of-SSA register/stack preservation, or
  before-return materialization handled by ideas 501-503.
- Inferring homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- Final RV64 select-publication admission consumes structured stack-home intent
  fields rather than reconstructing authority.
- The pointer stack-source and GPR-to-stack-destination representative shapes
  materialize only when supported intent is available.
- Unsupported widths, large offsets, stack-to-stack shapes, missing concrete
  stack fields, and coordinate confusion remain fail-closed.
- Backend proof covers the final consumer paths and preserves existing backend
  tests.

## Reviewer Reject Signals

- Reject materialization that bypasses `EdgePublicationMoveIntent` and derives
  homes from raw BIR, testcase names, object output, final registers, or target
  behavior.
- Reject folding generic immediate materialization from `src/pr37924.c` into
  this select-publication consumer.
- Reject adding stack-to-stack select-publication without a clear scratch and
  interleaving policy.
- Reject expectation, allowlist, unsupported-marker, pass/fail accounting, or
  runtime-comparison changes as proof of progress.
