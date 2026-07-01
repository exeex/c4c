# Select-Publication Move-Bundle Evidence And Authority

Status: Closed
Type: Focused producer/evidence splitter idea
Parent: `ideas/open/495_prepared_move_bundle_materialization_bucket_review.md`
Source Evidence:
- `build/agent_state/495_step2_move_bundle_coherence/summary.md`
- `build/agent_state/495_step2_move_bundle_coherence/classification.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/owner_matrix.tsv`
- `build/agent_state/495_step2_move_bundle_coherence/representative_classification.tsv`
Owning Layer: Prepared/BIR move-bundle evidence publication before RV64 consumption

## Goal

Publish or route enough select-publication move-bundle evidence to decide
whether the three ambiguous select-publication rows are coherent RV64-lowerable
move bundles or producer-authority gaps.

## Why This Exists

Idea 495 Step 2 found three `select_publication_move_bundle` rows whose current
case-log tokens omit event kind, phase, authority, parallel-copy status,
execution site, destination storage, and source storage. The representative row
is `src/builtin-constant.c`.

Those rows must not be treated as RV64-lowerable merely because they sit inside
the move-bundle bucket. They need a focused evidence/authority packet before a
target materializer can consume them.

## In Scope

- Audit the select-publication prepared/BIR record stream and case-log event
  publication for the missing move-bundle tokens.
- Publish durable event/phase/authority/storage details when the underlying
  producer facts are available.
- Split any resulting coherent select-publication move materialization work
  into a separate RV64 consumer idea.
- Preserve unavailable statuses when select-publication authority is missing,
  ambiguous, contradictory, or only visible through raw testcase shape.

## Out Of Scope

- RV64 materialization of select-publication move bundles before authority is
  explicit.
- Before-instruction, out-of-SSA, or before-return coherent move
  materialization.
- Inferring select-publication authority from source names, case-log absence,
  raw BIR shape, or target output.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Acceptance Criteria

- The three select-publication rows have explicit event, phase, authority,
  parallel-copy, execution-site, and storage evidence, or remain clearly
  unavailable with distinguishable statuses.
- Any coherent lowerable select-publication family is split into a separate
  RV64 materialization idea rather than implemented inside this evidence
  packet.
- Missing or ambiguous select-publication evidence remains fail-closed.
- Focused proof covers the evidence publication path and preserves backend
  behavior.

## Reviewer Reject Signals

- Reject RV64 code that lowers the three select-publication rows before
  explicit authority is published.
- Reject treating absent case-log tokens as proof that default move-bundle
  semantics apply.
- Reject testcase-name, source-shape, raw BIR, object-output, or final-register
  inference as evidence.
- Reject combining this producer/evidence repair with before-instruction,
  out-of-SSA, before-return, or other RV64 materialization work.
- Reject expectation, allowlist, unsupported-marker, pass/fail accounting, or
  runtime-comparison changes as proof of progress.

## Completion

Closed after Step 4 residual disposition. Idea 504 was an evidence/authority
idea, not an RV64 materialization idea:

- Step 1 inspected the three ambiguous select-publication rows and found that
  lower prepared facts existed, but the early RV64 rejection path lacked
  structured evidence.
- Step 2 published structured select-publication rejection evidence while
  preserving the existing fail-closed admission predicate.
- Step 3 classified the three rows with the new evidence and found no coherent
  final RV64 select-publication consumer packet ready from this row set.

Residual routing:

- `src/builtin-constant.c` and `src/pr58726.c` are real
  select-publication evidence rows, but need lower stack-source and
  stack-destination intent support before any RV64 select-publication consumer
  is ready. That follow-up is recorded in
  `ideas/open/505_select_publication_stack_home_intent_support.md`.
- `src/pr37924.c` is no longer a select-publication row. It is a generic
  out-of-SSA `phi_join_immediate_materialization` failure and is recorded in
  `ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md`.

Any downstream RV64 gcc-torture admission reclassification or broader bucket
refresh should be handled as a separate lifecycle initiative, not by expanding
this evidence/authority idea.
