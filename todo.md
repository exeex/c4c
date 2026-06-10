Status: Active
Source Idea Path: ideas/open/165_bir_comparison_condition_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add BIR Annotation Records

# Current Packet

## Just Finished

Completed Step 2 for
`ideas/open/165_bir_comparison_condition_annotation_schema.md`.
Added typed Route 7 BIR comparison/condition annotation records and
construction helpers without switching production consumers.

- Added explicit Route 7 statuses for available, missing block/instruction,
  wrong instruction, non-comparison, missing condition value, missing operand
  producer, duplicate producer, absent provenance, and no-match cases.
- Added operand roles and branch-condition semantic categories.
- Added `Route7ComparisonOperandRecord`,
  `Route7ComparisonInstructionRecord`, and `Route7BranchConditionRecord`.
- Added construction helpers:
  `bir::route7_comparison_operand_record(...)`,
  `bir::route7_comparison_instruction_record(...)`, and
  `bir::route7_branch_condition_record(...)`.
- Records reuse Route 1 source value identity and existing
  `ComparisonProducerKind`; they do not copy prepared fused-compare or
  materialized-condition producer records as BIR payloads.
- Focused tests compare Route 7 records with prepared fused-operand and
  materialized-condition oracles for select/folded operands, immediate
  operands, materialized condition producers, branch-condition provenance,
  non-comparison, missing/after-boundary producer, and duplicate producer
  cases.

## Suggested Next

Execute Step 3 by adding function-local Route 7 lookup/index helpers over the
new BIR comparison/condition records.

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
- Step 2 intentionally did not switch MIR, target/codegen, or prealloc
  production consumers.
- Step 3 should keep indexes rebuildable from BIR instruction/terminator
  records and avoid prepared-owned lookup payloads.

## Proof

Exact delegated Step 2 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
