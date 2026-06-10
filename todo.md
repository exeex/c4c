Status: Active
Source Idea Path: ideas/open/165_bir_comparison_condition_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Comparison And Condition Surfaces

# Current Packet

## Just Finished

Completed Step 1 inventory for
`ideas/open/165_bir_comparison_condition_annotation_schema.md`.

Prepared/oracle surfaces:

- `prepare::PreparedBranchCondition` / `PreparedBranchConditionKind`
  publishes branch condition kind, condition value, optional predicate,
  compare type, lhs/rhs values, fuse marker, and true/false labels.
- `prepare::find_prepared_fused_compare_operand_producer_facts(...)`
  returns optional lhs/rhs `PreparedFusedCompareOperandProducer` facts for
  `PreparedBranchConditionKind::FusedCompare`.
- `prepare::find_prepared_fused_compare_operand_producer(...)` is the
  operand-level oracle for immediate, same-block producer, and folded integer
  constant identity.
- `prepare::find_prepared_materialized_condition_producer(...)` is the oracle
  for materialized condition value -> comparison `BinaryInst` identity.
- Existing BIR query surfaces are
  `bir::find_comparison_operand_producer(...)`,
  `bir::find_fused_compare_operand_producer_facts(...)`, and
  `bir::find_materialized_condition_producer_identity(...)`.
- Existing MIR surfaces are generic same-block producer/constant helpers
  (`mir::find_same_block_*`, `mir::evaluate_same_block_integer_constant(...)`);
  there is no dedicated public MIR comparison-condition query to migrate yet.

Concrete Route 7 schema targets:

- Add a `Route7ComparisonStatus` with explicit `Available`,
  `MissingBlock`, `MissingInstruction`, `WrongInstruction`,
  `NonComparison`, `MissingConditionValue`, `MissingOperandProducer`,
  `DuplicateProducer`, `AbsentProvenance`, and `NoMatch` style outcomes.
- Add a `Route7ComparisonOperandRole` for `Lhs`, `Rhs`, and
  `ConditionValue`.
- Add a `Route7ComparisonProducerKind` or reuse/mirror
  `ComparisonProducerKind` for target-neutral semantic producer kinds:
  immediate, load-local, load-global, cast, binary, select, unknown.
- Instruction annotation record target:
  `Route7ComparisonInstructionRecord` over a BIR `BinaryInst` with block
  label/id, instruction index, condition/result value identity, predicate,
  compare type/operand type, lhs/rhs value identity, lhs/rhs producer identity
  or integer constant, producer instruction/index/kind, and explicit status.
- Operand/value annotation record target:
  `Route7ComparisonOperandRecord` keyed by block label/id,
  before-instruction index, operand role, value name/type/kind, producer
  kind, producer instruction/index, produced value identity, folded integer
  constant, and explicit status.
- Terminator annotation record target:
  `Route7BranchConditionRecord` over a conditional terminator with block
  label/id, terminator condition value identity, branch condition kind
  (`FusedCompare` vs `MaterializedCondition` as semantic categories, not
  prepared enum payload), optional comparison instruction reference,
  lhs/rhs operand record references or embedded identities, true/false
  label/id, and explicit status.
- Reuse Route 1 producer/value identity where possible for produced values,
  instruction indices, source names/types, and integer constants; do not copy
  `PreparedFusedCompareOperandProducer`,
  `PreparedFusedCompareOperandProducerFacts`, or
  `PreparedMaterializedConditionProducer` as payloads.
- Exclude AArch64 condition codes, fused-compare legality,
  `can_fuse_with_branch`, branch spelling/emission policy, hazard handling,
  emitted-register state, final instruction records/errors, and final
  instruction order.

Optional index shape for Step 3:

- `Route7ComparisonConditionIndex` built from a BIR `Function`/`Block` with
  vectors for comparison instruction records, operand records, and branch
  condition records.
- Lookup keys should use stable BIR identifiers: block label/id,
  instruction index, condition value name/type, before-instruction boundary,
  operand role/value identity, and conditional terminator labels.
- Index helpers should answer fused-operand facts, materialized-condition
  producer identity, and branch-condition provenance from Route 7 records
  without prepared-owned lookup payloads.

Required oracle coverage:

- Positive: fused compare with select lhs and folded-constant rhs;
  immediate operand; rhs-only operand availability; materialized condition
  producer; production prealloc branch condition fixture.
- Negative: non-fused/materialized-bool path for fused operand lookup;
  non-comparison binary condition; after-boundary producer; missing producer;
  absent prepared/BIR provenance; duplicate same-block producer; wrong
  instruction/index; missing condition value; no-match value/type.
- Tests should keep prepared comparison/condition reads as equivalence oracles
  while proving BIR payloads are semantic records rather than prepared-record
  clones.

## Suggested Next

Execute Step 2 by adding typed Route 7 BIR comparison/condition annotation
records and construction helpers only.

## Watchouts

- Keep Route 7 annotations target-neutral and semantic.
- Do not import AArch64 condition codes, fused-compare legality decisions,
  branch spelling, hazard handling, emitted-register state, final instruction
  records/errors, final instruction order, or target compare/branch instruction
  selection.
- Do not copy `PreparedFusedCompareOperandProducer` or
  `PreparedMaterializedConditionProducer` records as the BIR schema shape.
- Preserve explicit materialized-condition, integer-constant operand,
  non-comparison, missing producer, absent-provenance, non-materializable, and
  no-match cases where applicable.
- The first code-changing packet should be limited to BIR records/helpers and
  focused oracle tests; do not switch MIR, target/codegen, or prealloc
  production consumers.

## Proof

Inventory-only Step 1. No build/test proof required.

Delegated Step 2 narrow proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.
