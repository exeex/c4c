# 188 Phase E Route 7 comparison view consumer migration

## Goal

Switch one comparison or branch operand provenance reader to the Route 7
comparison/materialized-condition view.

## Why This Exists

Phase D identified Route 7 provenance as a bounded semantic input for selected
comparison and branch consumers. The implementation should keep branch lowering
and ALU result policy target-owned.

Source: `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.

## In Scope

- One AArch64 fused-compare, materialized-condition, branch operand, or scalar
  comparison provenance reader.
- Route 7 condition/comparison provenance records or validated facade keyed by
  the selected consumer.
- Prepared comparison helpers, scalar producer/select-chain fallbacks, Route 7
  facade validation, and fused-compare oracle tests as public comparison
  surfaces.

## Out Of Scope

- Branch spelling, fused legality, condition-code selection, hazard handling,
  emitted-register state, final branch/compare record ordering, ALU result
  storage, or return-chain operand recovery.
- Deleting prepared comparison helpers.
- Folding return-chain lookup into Route 7.

## Acceptance Criteria

- The selected consumer reads Route 7 provenance where available and preserves
  prepared/target branch policy.
- Tests prove provenance equality for fused-compare, materialized condition,
  unfused fallback, absent-route, and invalid-reference cases.
- Return-chain remains outside this migration.

## Reviewer Reject Signals

- Target branch policy or ALU machine-record formation moves into BIR.
- Return-chain lookup is hidden under comparison provenance.
- The implementation relies on broad BIR scans, predecessor rescans, or name
  matching.
- Tests are weakened, or invalid-reference/fallback cases are removed.
- One selected consumer is claimed as route-wide comparison helper contraction.
