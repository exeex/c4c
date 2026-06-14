Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement the Prepared Metadata Agreement Boundary

# Current Packet

## Just Finished

Step 2 (`Implement the Prepared Metadata Agreement Boundary`) added the local
store-source producer metadata agreement boundary in
`src/backend/prealloc/publication_plans.cpp`.

Changed files:
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `todo.md`
- `test_after.log`

Completed work:
- Added `prepared_store_source_producer_metadata_agrees(...)` beside the
  existing source-producer result helper.
- `plan_prepared_store_source_publication(...)` now copies
  `source_producer_kind`, producer block/instruction metadata, and producer
  payload pointers only when the raw producer pointer agrees with the prepared
  destination access block/index, prepared stored value name, source home
  value name/kind, producer kind, payload pointer, and produced value name/type.
- Left unrelated prepared planner policy in the existing collection path:
  status, intent, destination access, source-home discovery, source storage
  encoding, recovered/direct-global/byval/pointer-base, pending, duplicate,
  and output/baseline behavior were not rewritten.
- Added `verify_store_source_producer_metadata_requires_prepared_agreement()`
  to cover the positive agreement path and immediate fail-closed rows for
  missing source home/name, mismatched source home, unsupported home kind,
  mismatched source value name/type, unknown producer kind, wrong producer
  block, stale producer instruction index, missing payload, wrong payload
  kind, and produced value name/type drift.

## Suggested Next

Step 3 should add population-level coverage around
`populate_store_source_publication_plans(...)` proving that real prepared
module store-source records preserve the new fail-closed producer metadata
boundary when the producer lookup row drifts. Suggested focused gaps:
positive populated store-local/store-global records with agreed producer
metadata, plus fail-closed rows for stale producer rows and payload/result
identity drift without changing output expectations or diagnostics.

## Watchouts

- The Step 2 guard intentionally gates producer metadata publication; it does
  not change `PreparedStoreSourcePublicationStatus::Available` or ordinary
  source-home/storage metadata publication for otherwise available plans.
- Producer instruction agreement is enforced as same prepared block as the
  store destination access and strictly before the store access index.
- No changes were made to `publication_plans.hpp`, `plan.md`, source ideas,
  review artifacts, baselines, diagnostics, or expectation files.

## Proof

Ran delegated proof:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
