Status: Active
Source Idea Path: ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Migrate A Low-Risk Query Consumer

# Current Packet

## Just Finished

Completed Step 4 for
`ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md`.
Migrated the narrow shared MIR select-chain query consumer in
`src/backend/mir/query.cpp` to answer from rebuilt BIR Route 2
`Route2SelectChainValueIndex` records:

- `find_bir_select_chain_source_producer(...)` now maps the Route 2 root
  producer record into the existing MIR same-block producer identity.
- `find_bir_select_chain_direct_global_dependency(...)` now maps the Route 2
  direct-global dependency record instead of walking select/cast/binary
  producers recursively.
- `find_bir_select_chain_scalar_materialization_eligibility(...)` now reads the
  Route 2 scalar-materialization availability bit.
- `find_bir_select_chain_identity(...)` now assembles the public MIR identity
  from a single Route 2 value record.

Removed the old private recursive select-chain dependency walk from the shared
query path. Corrected the Route 2 lookup key so an explicit non-void
`BirSelectChainIdentityRequest::root_value_type` overrides
`root_value->type` when building the BIR lookup value.

Existing `backend_prepared_lookup_helper` coverage continues to use
prepared/prealloc results as oracle checks for direct-global success, explicit
no-dependency, select-root, non-select-root, before-boundary, type mismatch, and
missing-root cases. Added a focused fail-closed case where `root_value` points
at an existing root but `root_value_type` is an explicit mismatched type. No
AArch64 codegen files or prealloc production helpers were edited.

## Suggested Next

Execute Step 5 by adding any final target-neutral validation/cleanup needed for
the Route 2 schema migration, or request review if the runbook is exhausted.

## Watchouts

- Keep Route 2 annotations target-neutral.
- Do not import AArch64 move/branch choices, materialization cost, hazards,
  register availability, publication routing, or call ABI behavior.
- Do not claim progress through prepared-helper reshuffling unless BIR owns a
  typed annotation payload.
- Preserve explicit negative cases for no direct-global dependency and
  non-select-root behavior.
- Do not encode AArch64 select-materialization instruction spelling, label
  generation, scratch register selection, materialization cost, hazard policy,
  or call ABI placement in the Route 2 BIR schema.
- The existing `CallArgumentDirectGlobalSelectChainDependency` is a useful
  migration quarantine, but Step 2 should not broaden into call consumer
  switching.
- Step 4 rebuilds a Route 2 index per shared MIR select-chain query request.
  That is intentionally low-risk for migration; later work can cache or thread
  an index if a production hot path needs it.
- Route 2 select-chain lookup must continue to honor explicit non-void request
  types over the pointed `root_value` type.
- `Route2SelectChainDirectGlobalDependencyRecord::available == true` with
  `contains_direct_global_load == false` is the explicit no-dependency state
  for roots such as `LoadLocalInst`; an unavailable Route 2 value record remains
  the absent/fail-closed state for missing roots or before-producer queries.
- The shared MIR query consumer is now switched, but target/AArch64 policy
  remains outside the BIR Route 2 schema and was not edited in this packet.

## Proof

Exact delegated proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log`.
