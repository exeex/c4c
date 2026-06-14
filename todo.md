Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Recovered-Source Helper Surface

# Current Packet

## Just Finished

Step 1 located the recovered-source identity helper surface for the
`store_source_publications` packet.

- Prepared recovered-source helper:
  `prepare::find_prepared_recovered_narrow_store_source_for_wide_local_load(...)`
  in `src/backend/prealloc/publication_plans.cpp`, declared in
  `src/backend/prealloc/publication_plans.hpp`.
- Prepared same-block stored-value helper:
  `prepare::find_prepared_same_block_load_local_stored_value_source(...)` in
  `src/backend/prealloc/prepared_lookups.cpp`, declared in
  `src/backend/prealloc/addressing.hpp`.
- BIR identity helper:
  `mir::find_bir_same_block_load_local_stored_value_source_identity(...)` in
  `src/backend/mir/query.cpp`, declared in `src/backend/mir/query.hpp`.
- Primary consumer boundary:
  `prepared_recovered_narrow_store_source(...)` feeding
  `plan_store_local_source_publication(...)` in
  `src/backend/mir/aarch64/codegen/memory.cpp`; the prepared aggregate
  population path also calls the recovered helper inside
  `populate_store_source_publication_plans(...)`.
- Existing focused helper-test surface:
  `prepared_and_bir_same_block_load_local_stored_source_match(...)` and
  `verify_route3_load_local_stored_value_source_matches_prepared_or_falls_back()`
  in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Suggested Next

Execute Step 2 by adding a local prepared/BIR agreement boundary for recovered
narrow-store source lookup.

Owned files for the next packet should be
`src/backend/prealloc/publication_plans.cpp` and
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`; add
`src/backend/prealloc/publication_plans.hpp`,
`src/backend/prealloc/addressing.hpp`, or
`src/backend/prealloc/prepared_lookups.cpp` only if a small reusable/test-visible
agreement helper is needed.

## Watchouts

- Keep this packet limited to the recovered-source identity candidate.
- Do not absorb the byval pointer-source, direct-global select-chain, or
  source-value/source-producer metadata candidates.
- Preserve prepared publication planner policy, prepared addressing authority,
  public prepared aggregate compatibility, and current fail-closed behavior.
- Do not rewrite output expectations, diagnostics, helper statuses, or target
  output to claim progress.
- Current prepared recovered-source fail-closed behavior returns `std::nullopt`
  for missing block, non-integer load result, missing addressing, missing or
  non-matching prepared load access, missing frame slot, no prior matching
  store, non-matching store access, non-overlapping lane, wrong store width
  (`store_bits` missing, not 8 bits, or not narrower than the load), and all
  exhausted searches.
- The BIR helper is evidence only: it requires a valid block/root request, a
  matching block label when supplied, non-void root type, Route 3 load/store
  range evidence, load/store local instructions, local-slot MIR memory-access
  identities, and matching loaded root value name.
- Surfaces explicitly out of scope for Step 2: byval formal pointer-source
  classification, direct-global select-chain dependency lookup, source-value
  and source-producer metadata packets, target output/diagnostics/baselines,
  store publication status semantics, frame-slot policy, pointer-base homes,
  pending publication policy, and duplicate-publication handling.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. Build was up to date and
`backend_prepared_lookup_helper` passed 1/1.

Proof log: `test_after.log`.
