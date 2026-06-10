Status: Active
Source Idea Path: ideas/open/164_bir_call_use_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Lookup/Index Helpers

# Current Packet

## Just Finished

Completed Step 3 for
`ideas/open/164_bir_call_use_source_annotation_schema.md`.
Added function-local Route 6 lookup/index helpers over the Step 2 BIR
call-use annotation records without switching production consumers:

- Added `Route6CallUseSourceIndex`, rebuilt from BIR `Function`/`Block`
  `CallInst` payloads and value records.
- The index groups argument source, argument producer/materialization,
  direct-global, publication/memory source, result, and result-lane records.
- Added lookup helpers keyed by stable BIR block label/id, call instruction
  index, callee spelling, argument index, result value name/type, and
  result-lane value name/type.
- The helpers remain target-neutral indexes over Route 6 records, not
  prepared call-plan containers.

Focused test additions now prove indexed argument success, direct-global
dependency success, Route 3 memory-source reuse, result provenance success,
result-lane success/no-match, wrong-call behavior, missing-source behavior,
duplicate relationship/lane behavior, and ABI-bound exclusion behavior.

## Suggested Next

Execute Step 4 by migrating the narrowest shared Route 6 query consumer to
read Route 6 BIR records/index helpers while preserving prepared/BIR oracle
checks.

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
- Step 3 intentionally did not switch MIR, target/codegen, ABI assignment, call
  lowering, aggregate transport, or prealloc production consumers.
- Route 6 indexes are rebuilt from BIR records and must remain target-neutral;
  do not add prepared call-plan, ABI placement, outgoing-stack, aggregate
  transport, or register/storage fields to the index payload.

## Proof

Exact delegated Step 3 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
