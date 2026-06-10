Status: Active
Source Idea Path: ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broaden Validation And Prepare Closure

# Current Packet

## Just Finished

Completed Step 5 for
`ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md`.
Ran broader backend acceptance validation for the Route 2 select-chain schema
migration. The exact broad backend command completed successfully and
`test_after.log` now records `179/179` backend tests passed with no failures.

Closure readiness notes:

- Route 2 select-chain records and index helpers are covered by prepared/prealloc
  oracle checks for direct-global success, explicit no-dependency, select-root,
  non-select-root, before-boundary, type mismatch, explicit request-type
  mismatch, and missing-root cases.
- The shared MIR select-chain query consumer reads rebuilt BIR Route 2 index
  records.
- No implementation files were edited in Step 5.
- No expectation downgrades were introduced in this validation packet.
- No target-policy leaks were introduced; AArch64 behavior remains outside the
  BIR Route 2 schema.
- No helper-only reshuffle is being claimed as progress; the accepted route
  remains BIR-owned typed annotation payload plus a migrated shared query
  consumer.

## Suggested Next

Request reviewer/plan-owner closure for
`ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md` using
the current implementation diff, canonical `todo.md`, and broad backend
`test_after.log` proof.

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
- Step 5 was validation/status only; implementation and tests were intentionally
  left untouched.

## Proof

Exact delegated proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: `179/179` backend tests passed, `0` failed.

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log`.
