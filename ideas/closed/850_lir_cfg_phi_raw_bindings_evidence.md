# LIR CFG and PHI Raw-Binding Evidence

Status: Closed - evidence complete, no direct 734 handoff
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

## 866 Reconciliation And 734 Return

Idea 866 reuses this as the ordered documentation successor for CFG/PHI and
terminator raw-binding seams. It may return to 734 only after it proves an exact
typed handoff or identifies a separately scoped producer successor; accepted
734 CFG/PHI rows must not be reopened.

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

## Closure Note

Close accepted as documentation/evidence complete. Commit `3c314634a` created
the required docs directory and exactly the required files:

- `docs/lir_cfg_phi_raw_bindings_evidence/index.md`
- `docs/lir_cfg_phi_raw_bindings_evidence/01_cfg_phi_raw_binding_route.md`

The evidence maps `LirPhi` and terminator forms through LIR fields, verifier
checks, Raw-BIR importer paths, and accepted receiver tests. Current modeled
PHI, direct branch, conditional branch, switch, return, indirect branch, and
unreachable forms carry native block/value/type/edge authority where supported.

Conclusion: accepted 734 bounded receipts already cover the modeled CFG/PHI
Raw-BIR receiver rows described by this evidence. 850 does not authorize a new
direct 734 receiver handoff. If a future raw CFG/PHI seam appears, a separate
producer/verifier owner must first publish exact native block/value/type/edge
facts and malformed-boundary proof.

Return relation:

- 734 remains downstream with no new receiver row authorized by this idea.
- 797 remains downstream of any future 734 disposition.
- Text-based reconstruction of block labels, value names, predecessor labels,
  case labels, or LLVM terminator spelling remains rejected as authority.

Accepted proof:

```sh
find docs/lir_cfg_phi_raw_bindings_evidence -maxdepth 1 -type f -printf '%f\n' | sort
git diff --check
```

Result: PASS. The directory contained exactly
`01_cfg_phi_raw_binding_route.md` and `index.md`, and no implementation files
were modified.
