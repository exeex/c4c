# BIR Call Metadata Semantic Producer Admission

Status: Open
Type: Producer implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR semantic producer

## Goal

Repair BIR call metadata semantic producer admission for the `55` exact
current `semantic lir_to_bir` rows classified as direct-call and call-return
metadata failures.

## Why This Exists

The evidence reconstruction found `52` direct-call rows and `3` call-return
rows with visible first-topic BIR semantic call failures. These rows need call
callee, argument, and return-result metadata from the BIR producer before
prepared or RV64 consumers can safely proceed.

## In Scope

- Inspect `src/backend/bir/lir_to_bir/calling.cpp` and adjacent semantic call
  emission code for missing callee, argument, and result metadata.
- Add focused BIR tests for direct-call metadata and call-return metadata.
- Repair the BIR call producer without folding call failures into generic
  local-memory or scalar lanes.
- Prove with focused BIR tests and RV64 representatives including
  `src/20000412-2.c` and `src/20050121-1.c`, or current stronger substitutes.

## Out Of Scope

- Downstream call lowering assumptions in RV64/MIR.
- Treating call metadata as generic scalar/local-memory support.
- Runtime/intrinsic memory-effect repairs.
- Expectation rewrites, unsupported downgrades, allowlist changes, or weaker
  semantic admission checks.

## Acceptance Criteria

- Direct-call callee and argument metadata are published in the semantic BIR
  form expected by admission.
- Call-return result metadata is handled or explicitly split with evidence if
  it proves to require a separate producer boundary.
- Focused BIR tests cover both direct-call and call-return behavior.
- A narrow RV64 subset proves the current call rows advance without consumer
  inference.

## Reviewer Reject Signals

- Reject proving only direct-call while leaving call-return unexamined.
- Reject downstream call lowering that guesses missing BIR call facts.
- Reject changing call diagnostics, expectations, unsupported markers, or
  allowlists instead of repairing call metadata publication.
- Reject merging this lane into local-memory work without code and row
  evidence proving a shared producer boundary.
- Reject broad call-route rewrites that retain the same semantic admission
  failure mode.
