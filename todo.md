Status: Active
Source Idea Path: ideas/open/172_route6_call_use_semantic_source_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove The Narrow Migration

# Current Packet

## Just Finished

Completed plan Step 3 by migrating the selected AArch64 scalar call-argument
source-producer materialization consumer to read semantic source-producer facts
from an indexed Route 6 call-use source record when one is available.

`lower_scalar_call_argument_producers()` now builds a current-call
`Route6CallUseSourceIndex`, and
`find_scalar_call_argument_source_producer_materialization()` looks up the
argument producer through `route6_find_call_argument_source_producer()`.
Prepared source-producer lookup remains only as the fallback/oracle path when
the indexed Route 6 record is unavailable or incomplete. ABI/layout-bound reads
continue to use the prepared call argument plan.

## Suggested Next

Execute plan Step 4 by proving the narrow migration without treating the green
subset as broad prepared-call cache contraction.

## Watchouts

- Keep ABI/layout reads prepared-call-plan owned.
- Do not copy prepared call plans, aggregate transport, or helper/carrier
  policy into BIR.
- Do not weaken call-boundary expectations to claim cache contraction.
- Do not broaden Step 2 or Step 3 into direct-global dependency,
  publication-source routing, indirect callee, result-source, or lane-source
  consumers; those are separate Route 6 classes.
- The selected AArch64 consumer now depends on indexed Route 6 records, but the
  local index is intentionally scoped to the current call to avoid rebuilding a
  full-function index for each call in scalability cases.
- Route 6 currently reports unnamed/immediate call-argument values for this
  producer path as `MissingSourceProducer`, not `MissingSourceValue`; Step 4
  should preserve current behavior unless the supervisor explicitly expands
  the implementation contract.

## Proof

Passed delegated Step 3 proof; canonical log path is `test_after.log`.

```bash
(cmake --build build --target c4cll backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_codegen_route_aarch64_prepared_call_boundary_scalability)$' --output-on-failure) > test_after.log 2>&1
```
