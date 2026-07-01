# RV64 Out-Of-SSA Phi Join Immediate Materialization

Status: Open
Type: Focused RV64 materialization idea
Parent: `ideas/closed/504_select_publication_move_bundle_evidence_authority.md`
Source Evidence:
- `build/agent_state/504_step3_select_publication_consumer_classification/summary.md`
- `build/agent_state/504_step3_select_publication_consumer_classification/rows.tsv`
- `build/agent_state/504_step3_select_publication_consumer_classification/classification_counts.tsv`
Owning Layer: RV64 object lowering for coherent generic out-of-SSA immediate moves

## Goal

Classify and materialize coherent generic out-of-SSA
`phi_join_immediate_materialization` moves only when prepared parallel-copy
authority and immediate value facts are explicit.

## Why This Exists

Idea 504 Step 3 proved that `src/pr37924.c` is not a select-publication row
after the new diagnostic evidence. It now routes through the generic prepared
move-bundle path as:

- `event_kind=pre_terminator_copies`
- `phase=block_entry`
- `authority=out_of_ssa_parallel_copy`
- `move_reason=phi_join_immediate_materialization`
- `fragment_status=generic_move_bundle_materialization_failed`

That row is outside idea 504 and should not be handled by select-publication
evidence or intent-support work.

## In Scope

- Classify generic out-of-SSA immediate move rows and confirm whether
  `src/pr37924.c` represents a broader coherent family.
- Consume explicit prepared out-of-SSA parallel-copy authority and immediate
  value facts when materializing.
- Preserve fail-closed statuses for missing edge identity, missing immediate
  facts, unsupported destination storage, cycles, or coordinate confusion.
- Add focused backend coverage for representative immediate materialization
  only after the facts prove coherent.

## Out Of Scope

- Select-publication evidence, intent support, or materialization.
- Before-instruction, out-of-SSA register/stack preservation, or before-return
  materialization already handled by ideas 501-503.
- Inferring immediates from testcase shape, source constants, raw BIR order, or
  target output without prepared value facts.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- Generic out-of-SSA immediate materialization is classified with row evidence
  before implementation.
- Any available path consumes prepared parallel-copy authority and explicit
  immediate facts.
- Missing or ambiguous immediate facts remain fail-closed.
- Focused backend proof covers the implemented immediate materialization
  family, if one is proven available.

## Reviewer Reject Signals

- Reject using select-publication evidence to justify generic immediate
  materialization.
- Reject constants inferred from testcase names, source snippets, raw BIR
  shape, object output, or target register behavior.
- Reject combining this idea with select-publication intent support or broad
  move-bundle rewrites.
- Reject expectation, allowlist, unsupported-marker, pass/fail accounting, or
  runtime-comparison changes as proof of progress.
