Status: Active
Source Idea Path: ideas/open/172_route6_call_use_semantic_source_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The Selected Consumer

# Current Packet

## Just Finished

Completed plan Step 2 by tightening
`verify_bir_call_argument_source_producer_materialization_lookup()` for the
selected AArch64 scalar call-argument source-producer materialization
consumer.

The Route 6 oracle now explicitly checks load-local and materializable binary
positive records for call site, call-instruction index, block identity, callee,
argument index, argument value, source value/name/id, register source
relationship, producer kind, producer instruction pointer/index, produced
value, and Route 1 materialization availability. It also covers the
nonmaterializable binary eligibility path.

Fail-closed coverage now includes missing source relationship, unnamed/immediate
argument source value with no producer, missing producer, producer-after-call,
duplicate argument-source relationship, wrong-call index lookup, no-match index
lookup, and ABI-bound source-selection exclusion.

## Suggested Next

Execute plan Step 3 by migrating the selected AArch64 scalar call-argument
source-producer materialization consumer to depend on the indexed Route 6
source-producer record, keeping the prepared helper only as an oracle/fallback
while proving equivalence.

## Watchouts

- Keep ABI/layout reads prepared-call-plan owned.
- Do not copy prepared call plans, aggregate transport, or helper/carrier
  policy into BIR.
- Do not weaken call-boundary expectations to claim cache contraction.
- Do not broaden Step 2 or Step 3 into direct-global dependency,
  publication-source routing, indirect callee, result-source, or lane-source
  consumers; those are separate Route 6 classes.
- The existing BIR helper
  `bir::find_call_argument_source_producer_materialization(...)` already reads
  Route 6-backed records for the selected semantic shape; Step 3 should make
  the AArch64 consumer depend on the indexed Route 6 source record and keep the
  prepared helper only as an oracle/fallback while proving equivalence.
- Route 6 currently reports unnamed/immediate call-argument values for this
  producer path as `MissingSourceProducer`, not `MissingSourceValue`; Step 3
  should preserve current behavior unless the supervisor explicitly expands
  the implementation contract.

## Proof

Passed delegated Step 2 proof; canonical log path is `test_after.log`.

```bash
(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```
