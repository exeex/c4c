# 147 Comparison prealloc fact owner

## Goal

Create or name a narrow prealloc comparison owner for fused-compare and
materialized-condition producer facts.

## Why This Exists

Idea 141 found comparison/materialized-condition facts over same-block
producers and integer constants in `prepared_lookups.*`, but no existing
concrete comparison owner. Implementation work should first name the comparison
boundary instead of vaguely moving "comparison lookups."

## In Scope

- Establish a concrete owner such as `src/backend/prealloc/comparison.hpp` with
  a matching implementation file, or a specifically named control-flow
  comparison owner if extending `control_flow.hpp` is the clearer repo-local
  pattern.
- Move `PreparedFusedCompareOperandProducer`.
- Move `PreparedFusedCompareOperandProducerFacts`.
- Move `PreparedMaterializedConditionProducer`.
- Move fused-compare operand producer and materialized-condition producer lookup
  helper declarations.
- Depend on the same-block/source-producer materialization owner from idea 144.

## Out Of Scope

- Moving AArch64 compare instruction selection or condition-code emission into
  prealloc.
- Using `control_flow.hpp` as a dumping ground without naming the comparison
  boundary.
- Changing branch-condition semantics.

## Acceptance Criteria

- A concrete comparison owner is named before declarations are moved.
- Comparison facts move without target-local lowering policy.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full CTest if shared branch/comparison semantics change.

## Reviewer Reject Signals

- Follow-up fails to name a concrete owner and only says "clean comparison
  lookups".
- The owner move depends on line count rather than semantic comparison facts.
- Target-local compare lowering leaks into shared prealloc.
