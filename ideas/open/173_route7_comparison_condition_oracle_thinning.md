# 173 Route 7 comparison/condition oracle thinning

## Goal

Migrate one comparison or branch semantic consumer to Route 7 BIR comparison
records or selected route-index validation, then contract only prepared
comparison oracle/cache surface no longer needed by that consumer.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 7 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- Fused-compare operand producer facts.
- Materialized-condition producer identity.
- Comparison operand/source-producer semantic lookup helpers.
- One comparison or branch provenance consumer.

## Out Of Scope

- Target branch spelling, fused-compare legality, condition-code selection,
  hazards, emitted-register state, final instruction records/order, and AArch64
  compare/branch instruction selection.

## Acceptance Criteria

- The selected consumer reads `Route7ComparisonConditionIndex` or route-index
  validation for semantic provenance.
- Prepared comparison answers remain oracle-visible until equivalence is
  proven.
- Lhs/rhs producer, integer constant, materialized-condition, and negative
  cases are covered.

## Proof Route

Use fused-operand/materialized-condition oracle tests with producer and integer
constant coverage, then run the comparison/branch target subset.

## Reviewer Reject Signals

- Moving branch lowering policy into BIR.
- Treating fused-compare legality as schema.
- Contracting prepared comparison helpers before unexpanded consumers migrate.
