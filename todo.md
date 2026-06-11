Status: Active
Source Idea Path: ideas/open/172_route6_call_use_semantic_source_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Contract Only Proven Prepared Surface

# Current Packet

## Just Finished

Completed plan Step 5 by inspecting the selected AArch64 scalar call-argument
source-producer migration surface. No prepared semantic-source contraction is
safe in this packet: `lower_scalar_call_argument_producers` now builds a local
Route 6 call-use source index for the selected top-level call argument, but the
same materialization path still keeps prepared source-producer lookup as
fallback/oracle coverage and still uses it for recursive binary operand
materialization where the call-argument index is intentionally absent.

## Suggested Next

Supervisor should decide whether Step 5 is complete as a no-op handoff or
delegate a narrower follow-up after Route 6 has equivalent non-call recursive
producer lookup coverage.

## Watchouts

- Keep ABI/layout reads prepared-call-plan owned.
- Do not copy prepared call plans, aggregate transport, or helper/carrier
  policy into BIR.
- Do not weaken call-boundary expectations to claim cache contraction.
- Do not claim broad prepared-call cache contraction from the Step 4 green
  subset; it proves only the selected Route 6 oracle and narrow AArch64
  call-lowering path.
- No expectation downgrades were made or validated as part of Step 4.
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
- Do not remove
  `find_prepared_scalar_call_argument_source_producer_materialization` yet; it
  remains the fallback when indexed Route 6 call-use records are unavailable
  and the recursive producer oracle for binary operands below the call
  argument.
- The prepared lookup helper and prepare-frame contract tests still directly
  exercise `prepare::find_prepared_call_argument_source_producer_materialization`,
  so that prepared helper is not uniquely redundant to the migrated AArch64
  consumer.

## Proof

Passed delegated Step 5 proof; canonical log path is `test_after.log`.

```bash
(cmake --build build --target c4cll backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_codegen_route_aarch64_prepared_call_boundary_scalability)$' --output-on-failure) > test_after.log 2>&1
```

Result: 2/2 tests passed.
