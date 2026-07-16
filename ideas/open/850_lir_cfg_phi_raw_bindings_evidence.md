# LIR CFG and PHI Raw-Binding Evidence

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md`
Related:
- `docs/lir_string_authority_remaining_routes/handoff_to_813.md`
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`
Owning Layer: CFG/terminator/PHI operand and incoming-binding evidence
Routing Key: `cfg.phi-raw-bindings-evidence`

## Goal

Produce concrete evidence documents under `docs/lir_cfg_phi_raw_bindings_evidence/`
that map every `LirPhi` and terminator operand through producer, verifier, and
new-BIR receiver seams, including raw-binding gaps and malformed predecessor/
value-pair boundaries.

## Why This Exists

Block and value IDs exist where modeled, but the accepted input does not prove
coverage for raw terminator/PHI operand seams. 734 has accepted bounded CFG/PHI
receiver work but does not own or prove this family-wide seam map; 797 must
await accepted producer handoffs and 734 receipts. This idea first produces
evidence only and must not convert a raw seam into an implementation claim.

## Research Questions And Required Answer Files

There is 1 research question. The delivery must contain exactly 1 numbered
question-answer Markdown file plus one `index.md`.

1. `01_cfg_phi_raw_binding_route.md`

   Question: For every `LirPhi` and terminator operand/incoming binding, what
   native block/value/type relation reaches the verifier and BIR receiver, and
   which raw seams remain unproved?

   First diagnostic question: How does each `LirPhi` and terminator operand
   flow through verifier and BIR receivers, including malformed predecessor/
   value pairs?

   Required answer shape:
   - a per-form map from producer fields to verifier checks and BIR receiver
     paths, distinguishing native IDs from raw operands;
   - positive evidence plus malformed predecessor/value, foreign block/value,
     and type/edge mismatch evidence expectations where applicable;
   - an exact conclusion separating accepted 734 bounded receipts from
     remaining unproved raw seams;
   - a dependency/return statement requiring any later producer handoff to be
     accepted before 734 receipt and 797 convergence.

## Required Documentation Output

Create the research documents in:

```text
docs/lir_cfg_phi_raw_bindings_evidence/
```

Required files:

- `docs/lir_cfg_phi_raw_bindings_evidence/index.md`
- `docs/lir_cfg_phi_raw_bindings_evidence/01_cfg_phi_raw_binding_route.md`

`index.md` must link to the numbered answer file and summarize the result. It
must not replace the required answer file.

## In Scope

- Documentation and evidence tracing for the exact routing key only.
- Every `LirPhi` and terminator operand/incoming-binding seam through producer,
  verifier, and BIR receiver.
- Positive and applicable malformed, foreign, and mismatch evidence expectations
  for predecessor/value, block/value, type, and edge relations.
- The exact return order: evidence-backed producer handoff, then bounded 734
  receiver receipt, then 797 convergence input.

## Out Of Scope

- Implementation changes, activation, or any general operand/call/body-
  parameter authority work.
- Reopening accepted CFG/PHI rows, claiming whole CFG/PHI closure, or altering
  734's existing bounded receipts.
- Test expectation, unsupported-marker, allowlist, runtime-behavior, or
  lifecycle-state changes.
- Recovering block, value, type, owner, or edge facts from raw rendered text.

## Acceptance Criteria

- The output directory contains `index.md` and exactly the one numbered answer
  file named above.
- The answer gives a concrete map for every `LirPhi` and terminator form and
  labels native versus raw seams without omission.
- Positive evidence and applicable malformed predecessor/value, foreign
  block/value, and type/edge mismatch expectations are explicit.
- The documents preserve 734 as the receiver owner and 797 as downstream
  convergence, with no claimed receipt or implementation scope before an exact
  evidence-backed handoff.
- No implementation files, test expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed.

## Reviewer Reject Signals

- Reject any omitted `LirPhi` or terminator form, or a family-wide conclusion
  derived from selected accepted 734 rows.
- Reject text-based reconstruction of block, value, type, owner, predecessor,
  or edge identity.
- Reject a seam map without concrete producer/verifier/receiver evidence and
  applicable positive plus malformed/foreign/mismatch boundaries.
- Reject an implementation successor, testcase-shaped shortcut, expectation
  downgrade, helper rename, or classification-only change claimed as research
  progress.
- Reject a 734 receipt or 797 convergence claim that lacks an accepted exact
  producer handoff.
