Status: Active
Source Idea Path: ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Lookup/Index Helpers

# Current Packet

## Just Finished

Completed Step 3 for
`ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md`.
Added BIR-owned function-local Route 2 lookup/index helpers in
`src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp`:

- `Route2SelectChainValueIndex` stores rebuilt
  `Route2SelectChainValueRecord` payloads for one BIR block.
- `Route2SelectChainValueQuery` carries a function-local index plus the
  before-instruction boundary.
- `route2_build_select_chain_value_index(...)` rebuilds Route 2 value records
  from the block's BIR annotation construction helpers.
- `route2_find_select_chain_value_record(...)` performs target-neutral lookup
  over Route 2 records by named root value, value type, and before-boundary.

Extended `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` so the
Route 2 index is proved separately from prepared storage. Coverage now checks
direct-global success, explicit no-dependency local roots, select roots,
non-select nested roots, before-producer boundaries, type/name mismatches, and
missing roots. No production MIR, AArch64, or prealloc consumer was switched.

## Suggested Next

Execute Step 4 by adding the first low-risk consumer migration over the Route 2
BIR index, keeping prepared/prealloc helpers as oracle checks and avoiding
AArch64 target-policy leakage.

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
- Step 3 intentionally allows the Route 2 index builder to reuse Route 1
  construction helpers, but `route2_find_select_chain_value_record(...)`
  answers only from rebuilt Route 2 records.
- `Route2SelectChainDirectGlobalDependencyRecord::available == true` with
  `contains_direct_global_load == false` is the explicit no-dependency state
  for roots such as `LoadLocalInst`; an unavailable Route 2 value record remains
  the absent/fail-closed state for missing roots or before-producer queries.
- The new Route 2 index is function-local/block-backed and currently has no
  production consumer switch.

## Proof

Exact delegated proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log`.
