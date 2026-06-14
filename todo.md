Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Nearby Fail-Closed Rows

# Current Packet

## Just Finished

Step 3 (`Add Nearby Fail-Closed Rows`) added populated
`PreparedBirModule` coverage for the store-source source-value/source-producer
metadata boundary.

Changed files:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `todo.md`
- `test_after.log`

Completed work:
- Added `verify_populated_store_source_producer_metadata_fails_closed()` near
  the direct Step 2 helper.
- Covered positive populated store-local and store-global records with agreed
  binary producer metadata.
- Added populated fail-closed rows for stale producer-after-store lookup rows,
  duplicate producer rows, and produced-value type drift.
- Verified the fail-closed rows preserve store-source publication availability,
  ordinary source identity, global publication intent, stack-homes-only
  compatibility, and register source storage encoding.

## Suggested Next

Step 4 should run the supervisor-selected broader backend validation for this
metadata packet and decide whether the current slice is ready to commit.

## Watchouts

- This packet intentionally added fixture coverage only; no planner policy,
  output baselines, diagnostics, `publication_plans.cpp`, `publication_plans.hpp`,
  `plan.md`, or source idea files were changed.
- The populated stale-row case proves a producer that appears only after the
  store remains metadata-free while the store-source record is still available.
- The duplicate producer-row case exercises the existing producer lookup
  fail-closed collapse to `Unknown`.

## Proof

Ran delegated proof:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'; } > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.
