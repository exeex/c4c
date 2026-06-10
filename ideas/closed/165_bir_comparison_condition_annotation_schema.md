# 165 BIR comparison and condition annotation schema

## Goal

Prototype BIR instruction and terminator annotations for comparison and
materialized-condition producer identity established by Phase A Route 7.

## Why This Exists

Phase A proved BIR comparison producer queries against prepared fused-operand
and materialized-condition answers. Phase B classified comparison provenance as
instruction annotations and branch-condition provenance as terminator
annotations, while rejecting target branch policy and final instruction records.

## In Scope

- Define instruction annotations for comparison-producing `BinaryInst`,
  condition value name, lhs/rhs producer nodes or integer constants, producer
  instruction index, fused-operand provenance, and materialized-condition
  producer references.
- Define terminator annotations for branch-condition provenance that point to
  comparison or materialized-condition producer records.
- Keep prepared fused-compare operand and materialized-condition producer
  queries as migration oracles.
- Preserve negative cases where comparison provenance is absent or not
  materializable.

## Out Of Scope

- Fused-compare legality, condition-code selection, branch emission strategy,
  final instruction records/errors, hazard handling, emitted-register state,
  and AArch64 compare/branch instruction selection.

## Acceptance Criteria

- BIR annotations answer operand and condition producer provenance without
  owning target compare/branch lowering policy.
- Existing prepared comparison queries remain available as oracle proof during
  migration.
- Terminator annotations expose condition provenance through stable BIR ids
  rather than target condition-code names.

## Reviewer Reject Signals

- Reject if annotation payloads include AArch64 condition codes, fused-compare
  legality decisions, branch spelling, hazard handling, emitted-register state,
  final record errors, or final instruction order.
- Reject if `PreparedFusedCompareOperandProducer` or
  `PreparedMaterializedConditionProducer` records are copied wholesale into BIR.
- Reject if only a single fused-compare testcase passes while materialized
  condition, integer-constant operand, non-comparison, and negative provenance
  cases remain unexamined.
- Reject expectation downgrades that mark comparison/condition supported-path
  behavior unsupported without explicit user approval.
- Reject broad branch lowering rewrites as schema progress.

## Closure Notes

Closed after implementing Route 7 BIR comparison operand, comparison
instruction, and branch-condition records/indexes over instruction,
terminator, and value payloads, oracle tests for fused-operand and
materialized-condition answers, and a low-risk migration of
`bir::find_materialized_condition_producer_identity(...)` to Route 7
records/index helpers.

Reviewer report `review/165_bir_comparison_condition_route_review.md` found no
blocking issues and recommended closure. The residual risk is limited to
future consumer expansion: later Route 7 users must continue excluding target
branch spelling, fused-compare legality/can-fuse policy, condition-code
selection, hazard handling, emitted-register state, and final instruction
records/order from BIR schema payloads.

Closure validation used canonical broad backend logs from:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

`test_before.log` and `test_after.log` both passed `179/179`, and the
regression guard passed with no new failures or pass-count regression.
