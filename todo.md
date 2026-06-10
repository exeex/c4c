Status: Active
Source Idea Path: ideas/open/165_bir_comparison_condition_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate A Low-Risk Query Consumer

# Current Packet

## Just Finished

Completed Step 4 for
`ideas/open/165_bir_comparison_condition_annotation_schema.md`.
Migrated the narrow shared BIR materialized-condition helper
`bir::find_materialized_condition_producer_identity(...)` to read Route 7
BIR records/index helpers while preserving the existing public result shape.

- Added a block-backed `Route7ComparisonConditionIndex` builder so the legacy
  helper can index the caller's original BIR block without pointer drift.
- The helper now looks up the materialized condition through
  `route7_find_materialized_condition(...)` and converts Route 7 operand
  records back into `ComparisonOperandProducer` values.
- Focused tests tie the migrated helper result to the Route 7 indexed
  materialized-condition record while retaining prepared/BIR oracle coverage
  for positive, materialized-condition, integer-constant, non-comparison,
  missing-producer, no-match, and duplicate-producer cases.
- No MIR API, target/codegen, or prealloc production consumers were switched.

## Suggested Next

Execute Step 5 by running broader backend acceptance validation and recording
closure readiness for idea 165.

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
- Step 4 intentionally did not switch MIR, target/codegen, or prealloc
  production consumers.
- `find_materialized_condition_producer_identity(...)` now depends on Route 7
  record/index semantics; future changes to Route 7 no-match/duplicate status
  handling can affect this legacy helper's fail-closed behavior.

## Proof

Exact delegated Step 4 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
