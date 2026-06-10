# 158 BIR comparison condition producer identity

## Goal

Add BIR-owned comparison and materialized-condition producer identity after the
producer/source identity foundation exists.

## Why This Exists

Phase A found comparison/materialized-condition producer facts to be
target-neutral provenance relationships over BIR compares, same-block
producers, and constants. AArch64 should keep fused-compare legality and branch
emission policy, but BIR should own the semantic producer identity.

Input artifact: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`.

## In Scope

- Add comparison relationships keyed by branch condition or condition value.
- Store lhs/rhs producer nodes or integer constants,
  comparison-producing `BinaryInst`, condition value name, and producer
  instruction index.
- Bridge fused-compare operand producer and materialized-condition producer
  queries.
- Switch comparison/branch consumers only for provenance reads after
  equivalence is proven.

## Out Of Scope

- Fused-compare legality, condition-code selection, branch emission strategy,
  final instruction records/errors, hazard handling, emitted-register state, or
  AArch64 compare/branch instruction selection.

## Acceptance Criteria

- BIR comparison queries match prepared fused-operand and
  materialized-condition producer answers for constants, same-block producers,
  binary compare values, and unavailable cases.
- Comparison/branch consumer switches remain limited to producer provenance.

## Proof Route

Run matching before/after backend comparison/branch coverage with equivalence
checks for fused operands and materialized conditions, including integer
constants and non-fusable negative paths.

## Reviewer Reject Signals

- Moves branch-fusion legality, condition-code emission, branch strategy,
  final instruction records, or emitted-register state into BIR.
- Proves a single compare testcase without nearby constant and
  materialized-condition coverage.
- Rewrites expectations instead of preserving existing target behavior through
  query-equivalence proof.

## Closure Notes

Closed after Step 6 acceptance validation. The completed route adds BIR
comparison producer queries, proves them against prepared fused-operand and
materialized-condition producer answers, and switches comparison/branch
provenance consumers without moving target policy into BIR.

Close-time regression guard used matching backend CTest logs:
`test_before.log` and regenerated `test_after.log` both reported 179 passing
backend tests and 0 failures. The guard passed with non-decreasing pass count,
which is appropriate for this lifecycle-only close operation.
