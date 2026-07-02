# BIR Runtime Intrinsic Memory Producer Admission

Status: Open
Type: Producer implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR semantic producer

## Goal

Repair BIR runtime/intrinsic memory producer admission for the `34` exact
current `semantic lir_to_bir` rows classified as `memcpy` and `memset`
memory-effect failures.

## Why This Exists

The evidence reconstruction found `19` `memcpy` rows and `15` `memset` rows
whose first visible failure is a runtime/intrinsic semantic producer topic.
These rows are related to memory behavior but should not be folded into generic
local-memory work unless producer inspection proves a shared boundary.

## In Scope

- Identify the BIR producer path responsible for `memcpy` and `memset`
  semantic memory-effect facts.
- Add focused BIR tests for intrinsic memory-effect fact publication for both
  intrinsic families.
- Repair `memcpy` and `memset` semantic admission without named-case runtime
  rewrites.
- Prove with focused tests and RV64 representatives including
  `src/20000703-1.c` and `src/20041218-1.c`, or current stronger substitutes.

## Out Of Scope

- Generic load, GEP, store, scalar/local-memory, or alloca producer work unless
  code inspection proves a shared helper must change.
- Runtime library substitution or target-specific call replacement as the main
  fix.
- Expectation rewrites, unsupported downgrades, allowlist changes, or weaker
  semantic admission checks.

## Acceptance Criteria

- `memcpy` and `memset` semantic memory effects are represented in the BIR
  facts required by admission.
- Focused BIR tests cover both intrinsic families.
- RV64 representative rows advance because the producer publishes facts, not
  because consumers infer or bypass them.
- If shared local-memory helpers are changed, the proof includes nearby
  generic local-memory coverage.

## Reviewer Reject Signals

- Reject proving `memcpy` while leaving `memset` unsupported or unexamined.
- Reject replacing intrinsic rows with named-case lowering or runtime call
  special cases.
- Reject folding this lane into local-memory without proof of a shared producer
  boundary.
- Reject expectation or unsupported-marker downgrades as producer progress.
- Reject helper renames that preserve the same runtime/intrinsic semantic
  admission failure.
