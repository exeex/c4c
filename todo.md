Status: Active
Source Idea Path: ideas/open/165_bir_comparison_condition_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Lookup/Index Helpers

# Current Packet

## Just Finished

Completed Step 3 for
`ideas/open/165_bir_comparison_condition_annotation_schema.md`.
Added function-local Route 7 lookup/index helpers over the Step 2 BIR
comparison/condition annotation records without switching production
consumers.

- Added `Route7ComparisonConditionIndex`, rebuilt from BIR `Function` blocks
  by collecting comparison instruction records, operand records, and
  branch-condition records.
- Added lookup helpers for comparison instruction, comparison operand,
  materialized condition, and branch-condition provenance keyed by stable BIR
  block label/id, instruction index, condition value, before-instruction
  boundary, operand role/value identity, and block terminator identity.
- Lookup helpers return Route 7 records with explicit statuses and remain
  target-neutral BIR indexes, not prepared comparison-plan containers.
- Focused tests prove indexed success for fused-operand/materialized-condition
  and branch-condition paths, plus indexed non-comparison, no-match,
  missing-producer, and duplicate-producer fail-closed statuses against the
  existing prepared/BIR oracles.

## Suggested Next

Execute Step 4 by migrating the narrowest shared comparison/condition helper
to read Route 7 BIR records/index helpers while preserving prepared/BIR oracle
coverage.

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
- Step 3 intentionally did not switch MIR, target/codegen, or prealloc
  production consumers.
- There is no dedicated public MIR comparison-condition query yet; Step 4 may
  need to migrate the narrowest existing BIR helper rather than inventing a
  broad MIR API.

## Proof

Exact delegated Step 3 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
