Status: Active
Source Idea Path: ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add BIR Annotation Records

# Current Packet

## Just Finished

Completed Step 2 for
`ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md`.
Added BIR-owned Route 2 schema records and construction helpers in
`src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp`:

- `Route2SelectChainProducerKind` and
  `Route2SelectChainProducerRecord` for typed root/direct producer
  instruction identity, instruction index, produced value, block identity, and
  direct `LoadGlobalInst` global identity.
- `Route2SelectChainDirectGlobalDependencyRecord` for explicit available
  direct-global summaries, including the present-negative no-dependency state,
  root-is-select, root instruction index, direct load instruction index, and
  load-global identity.
- `Route2SelectChainValueRecord` for root value identity, root value name,
  root-is-select, root instruction index, scalar materialization availability,
  root producer record, and embedded direct-global dependency record.
- `route2_select_chain_producer_kind(...)`,
  `route2_select_chain_producer_record(...)`, and
  `route2_select_chain_value_record(...)` construction/query helpers.

Extended `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` with
prepared-oracle comparison coverage for Route 2 records across select-root,
direct `LoadGlobalInst`, nested `BinaryInst`/`CastInst`, local no-dependency,
before-producer, mismatched type, and missing-root cases. No production MIR,
AArch64, or prealloc consumer was switched.

## Suggested Next

Execute Step 3 by adding function-local lookup/index helpers over the Route 2
records in `src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp`, then prove
the index is rebuildable from BIR annotation payloads and preserves
direct-global success, explicit no-dependency, select-root, non-select-root,
and fail-closed cases.

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
- Step 2 intentionally reused the existing Route 1 same-block producer index
  as a construction helper only. Step 3 should decide the Route 2 index shape
  without making Route 1 the durable Route 2 semantic owner.
- `Route2SelectChainDirectGlobalDependencyRecord::available == true` with
  `contains_direct_global_load == false` is the explicit no-dependency state
  for roots such as `LoadLocalInst`; an unavailable Route 2 value record remains
  the absent/fail-closed state for missing roots or before-producer queries.

## Proof

Exact delegated proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log`.
