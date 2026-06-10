Status: Active
Source Idea Path: ideas/open/164_bir_call_use_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate A Low-Risk Query Consumer

# Current Packet

## Just Finished

Completed Step 4 for
`ideas/open/164_bir_call_use_source_annotation_schema.md`.
Migrated the narrow shared BIR call-use helper
`bir::find_call_argument_source_producer_materialization(...)` to answer from
Route 6 BIR argument source-producer records instead of directly owning the raw
scan.

- Kept the old raw same-block scan as an internal primitive for Route 6 record
  construction, avoiding recursive record/helper calls.
- The public helper now converts the Route 6 argument producer/materialization
  record back into the existing `CallArgumentSourceProducerMaterialization`
  result shape.
- Focused tests tie the migrated helper answer to Route 6 indexed BIR records
  for materializable producer, nonmaterializable producer, missing producer,
  and duplicate relationship fail-closed cases.
- No MIR, target/codegen, ABI assignment, call lowering, aggregate transport,
  or prealloc production consumers were switched.

## Suggested Next

Execute Step 5 by running the broader backend acceptance validation and
recording closure readiness for idea 164.

## Watchouts

- Keep Route 6 annotations target-neutral and semantic.
- Do not import ABI register names, stack slots, outgoing stack area sizing,
  variadic FPR counts, clobber/preservation sets, byval lane layout, scratch
  resources, helper/carrier protocol, destination homes, aggregate transport
  layout, or ABI/layout-bound publication-routing source-selection reads.
- Do not copy `PreparedCallArgumentPlan`, call result plans, outgoing-stack
  plans, or aggregate transport lane records as the BIR schema shape.
- Preserve explicit result, direct-global, memory/publication-source,
  wrong-call, missing-source, ABI-bound exclusion, and no-match cases where
  applicable.
- Existing `CallArgumentSourceSelection` still contains quarantine fields for
  prepared-only layout comparisons. Route 6 records keep those ABI/layout
  fields out of the schema and use them only to report `AbiBoundExcluded`.
- Route 6 indexes are rebuilt from BIR records and must remain target-neutral;
  do not add prepared call-plan, ABI placement, outgoing-stack, aggregate
  transport, or register/storage fields to the index payload.
- The migrated public helper preserves legacy fail-closed behavior by returning
  unavailable for Route 6 negative statuses such as missing producer and
  duplicate relationship.

## Proof

Exact delegated Step 4 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
