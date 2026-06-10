Status: Active
Source Idea Path: ideas/open/164_bir_call_use_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add BIR Annotation Records

# Current Packet

## Just Finished

Completed Step 2 for
`ideas/open/164_bir_call_use_source_annotation_schema.md`.
Added typed Route 6 BIR call-use annotation records and construction helpers
without switching production consumers:

- Added Route 6 status/source-kind/value-role vocabularies plus typed records
  for call argument source identity, argument producer/materialization,
  argument direct-global dependency, argument publication/memory source,
  result provenance, and result-lane provenance.
- Added construction helpers over `CallInst` and BIR block context:
  `route6_call_argument_source_record(...)`,
  `route6_call_argument_source_producer_record(...)`,
  `route6_call_argument_direct_global_dependency_record(...)`,
  `route6_call_argument_publication_source_record(...)`,
  `route6_call_result_source_record(...)`, and
  `route6_call_result_lane_source_record(...)`.
- Reused Route 1 producer/value/materialization records, Route 2 direct-global
  dependency records, Route 3 memory access records, and Route 4/5 publication
  record slots where applicable instead of duplicating producer, memory, or
  publication schemas.
- Explicit statuses now cover wrong/missing call, missing argument/result,
  duplicate argument relationship, duplicate result lane, missing source
  producer, missing direct-global, missing memory/publication source,
  no-match, and ABI-bound exclusion.

Focused test additions keep prepared/BIR helper reads as oracles and cover
argument producer/materialization, direct-global dependency, Route 3 memory
source reuse, result/result-lane provenance, ABI-bound exclusion, duplicate
relationship/lane, missing argument/result, wrong call, and missing producer
cases.

## Suggested Next

Execute Step 3 by adding function-local Route 6 lookup/index helpers over the
new call-use annotation records.

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
- Step 2 intentionally did not switch MIR, target/codegen, ABI assignment, call
  lowering, aggregate transport, or prealloc production consumers.
- Step 3 indexes should be rebuilt from Route 6 records and should not become
  prepared call-plan containers.

## Proof

Exact delegated Step 2 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`1/1` matching backend test passed).
