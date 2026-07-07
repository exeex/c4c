# BIR Scalar Local-Memory Semantic Admission Follow-Up

Status: Open
Activation Priority: Deferred until the supervisor selects the scalar/local-memory lane.
Type: Downstream semantic admission follow-up
Parent: `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
Owning Layer: BIR scalar/local-memory producer

## Goal

Repair the scalar/local-memory semantic admission failures exposed after
scalar-control-flow representatives advanced beyond their original BIR
admission diagnostics.

## Why This Exists

The scalar-control-flow lifecycle proved that the IEEE comparison
representatives no longer stop at the original scalar-control-flow producer
boundary. Rows such as `src/ieee/fp-cmp-8.c`, `src/ieee/fp-cmp-8f.c`,
`src/ieee/fp-cmp-8l.c`, and `src/ieee/pr38016.c` now fail in the
`scalar/local-memory semantic family`. That is a distinct producer/lowering
boundary and should be handled outside the scalar-control-flow runbook.

## In Scope

- Identify the BIR scalar/local-memory producer boundary responsible for the
  current IEEE-row diagnostics.
- Add focused BIR/backend coverage for the scalar/local-memory facts or
  fail-closed admission behavior.
- Repair the semantic producer or lowering rule that causes the current
  scalar/local-memory diagnostics.
- Prove the affected IEEE representatives advance because scalar/local-memory
  facts are correct.

## Out Of Scope

- Scalar-control-flow CFG, terminator, or phi producer repair.
- Function-signature producer repair.
- Scalar-binop producer repair already tracked by
  `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- RV64 object-lowering fragments tracked by
  `ideas/open/563_rv64_object_lowering_control_flow_fragments.md`.
- Expectation rewrites, unsupported downgrades, allowlist-only changes, or
  named IEEE-row shortcuts.

## Acceptance Criteria

- Focused coverage exists for the scalar/local-memory producer or lowering
  boundary responsible for the current diagnostics.
- The affected IEEE representatives advance beyond the
  `scalar/local-memory semantic family` blocker because semantic facts are
  produced or rejected fail-closed correctly.
- Any remaining failures are recorded under their own downstream owner
  boundary rather than folded into scalar-control-flow or scalar-binop work.

## Reviewer Reject Signals

- Reject named-case handling for only the IEEE representatives instead of a
  semantic scalar/local-memory producer or lowering rule.
- Reject expectation rewrites, unsupported marker changes, row
  reclassification, or allowlist-only changes claimed as capability progress.
- Reject changes that route the failures back into scalar-control-flow
  without focused evidence that the scalar-control-flow BIR facts regressed.
- Reject scalar-binop changes claimed as scalar/local-memory progress unless
  the failing diagnostics actually move to the scalar-binop owner boundary.
- Reject broad scalar rewrites that leave the exact same scalar/local-memory
  admission failure behind a renamed helper.
