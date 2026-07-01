# Select-Publication Move-Bundle Evidence And Authority

Status: Open
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
