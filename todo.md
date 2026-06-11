Status: Active
Source Idea Path: ideas/open/172_route6_call_use_semantic_source_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Route 6 Oracle Coverage

# Current Packet

## Just Finished

Completed plan Step 1 by selecting exactly one Route 6 migration boundary:
the AArch64 scalar call-argument source-producer materialization consumer,
centered on
`find_scalar_call_argument_source_producer_materialization(...)` and its
caller `materialize_scalar_call_argument_value(...)` in
`src/backend/mir/aarch64/codegen/calls.cpp`.

Selected consumer class:
`AArch64 scalar call-argument source-producer materialization`.

Route 6 semantic-source facts this consumer should read:

- `Route6CallArgumentSourceProducerRecord` from `Route6CallUseSourceIndex`
  via `route6_find_call_argument_source_producer(...)`.
- Argument source identity for the selected call argument: call site,
  call-instruction index, callee, argument index, source value/name, and
  register-like argument source relationship.
- Source-producer semantic facts: producer kind `LoadLocal` or `Binary`,
  producer instruction pointer, producer instruction index, produced value,
  and Route 1 materialization availability.
- Fail-closed semantic statuses: missing source relationship, missing source
  value, missing source producer, duplicate argument relationship, wrong call,
  and producer-after-call/no-match cases.

ABI/layout-bound negative cases that must stay prepared-call-plan owned:

- `PreparedCallArgumentSourceSelection` stack/frame details, including frame
  slot value/address source, local-frame address materialization, selected
  source bytes, source stack offset, source size/alignment, and address
  materialization frame-slot/byte-offset fields.
- Call argument ABI placement and move effects: register/stack assignment,
  outgoing stack area, before-call moves, byval register/stack lanes, scratch
  requirements, clobbers, helper/carrier protocols, destination homes, and
  aggregate transport layout.
- Publication-source routing and direct-global argument routing are adjacent
  Route 6 consumer candidates, but they are not part of this selected consumer
  unless the supervisor expands scope.

## Suggested Next

Execute plan Step 2 by adding or tightening Route 6 oracle coverage for the
selected scalar call-argument source-producer materialization class before any
consumer migration. The oracle should cover load-local and binary source
producer positives, nonmaterializable binary eligibility, missing/future
producer fail-closed cases, duplicate argument-source fail-closed behavior,
and ABI-bound source-selection exclusions.

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

## Proof

Inventory-only packet; no build, ctest execution, or proof log was required
or run.

Exact narrow proof route for Step 2 oracle coverage:

```bash
cmake --build build --target backend_prepared_lookup_helper_test
ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure
```

Exact narrow proof route for Step 4 migration proof:

```bash
cmake --build build --target backend_prepared_lookup_helper_test backend_codegen_route_aarch64_prepared_call_boundary_scalability
ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_codegen_route_aarch64_prepared_call_boundary_scalability)$' --output-on-failure
```
