# Route 7 comparison provenance diagnostic oracle

## Goal

Replace or add one visible diagnostic/oracle row for Route 7 comparison
provenance while keeping target branch policy and prepared fallback authority
outside BIR schema.

## Why This Exists

The Phase B2 readiness audit classifies Route 7 schema as sufficient for
selected comparison provenance readers, but diagnostic/oracle replacement
remains separate from branch-policy cleanup. The selected follow-up should
prove comparison provenance visibility without claiming ownership of fused
legality, condition-code selection, branch spelling, hazards, emitted-register
state, final instruction order, or AArch64 compare/branch instruction
selection.

This idea is the bounded follow-up for the Phase B2 Route 7 candidate:
selected comparison provenance reader or diagnostic row.

## In Scope

- Select exactly one comparison provenance reader or visible diagnostic/oracle
  row.
- Use Route 7 records or indexes for comparison-producing instruction,
  condition value/name, lhs/rhs producer nodes or integer constants, producer
  instruction index, fused-operand provenance, materialized-condition
  provenance, and branch-condition provenance.
- Preserve prepared fallback for fused compare, materialized condition, route
  debug/handoff, printer/debug, and target branch-policy oracle surfaces.
- Prove fused, materialized, unfused fallback, absent-route, invalid-reference,
  duplicate, and mismatch behavior.

## Out Of Scope

- Branch-policy cleanup or branch emission migration.
- Moving fused-compare legality, condition-code choice, branch spelling,
  hazards, emitted-register state, final instruction order, final instruction
  records/errors, or AArch64 compare/branch selection into BIR schema.
- Weakening prepared printer/debug, route-debug, or string-authority coverage.

## Acceptance Criteria

- One selected diagnostic/oracle row or reader obtains Route 7 comparison
  provenance from BIR.
- Prepared fallback remains for target branch policy and any policy-sensitive
  oracles.
- Proof covers fused, materialized, unfused fallback, absent-route,
  invalid-reference, duplicate, mismatch, and unchanged expected strings.

## Closure Note

Closed after the materialized-condition provenance reader was selected as the
Route 7 surface. The implementation validates Route 7 materialized-condition
producer identity, fails closed for absent, invalid, duplicate, and mismatched
facts, and preserves prepared/AArch64 authority for branch targets, final
branch spelling, condition suffix mapping, fused legality, hazards,
emitted-register state, final instruction order, and final assembler rows.

Close proof used:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Regression guard result: 180 passed before, 180 passed after, no new failures.

## Reviewer Reject Signals

- Reject a slice that treats comparison provenance as ownership of branch
  emission, fused legality, hazards, emitted-register state, final instruction
  order, final records/errors, or AArch64 compare/branch selection.
- Reject diagnostic/debug/handoff replacement without expected-string proof.
- Reject invalid-reference, duplicate, mismatch, or fallback coverage gaps.
- Reject branch-policy cleanup hidden inside a provenance adapter.
- Reject expected-string rewrites, unsupported downgrades, or helper renames
  claimed as capability progress.
