Status: Active
Source Idea Path: ideas/open/prealloc-publication-accessor-contracts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Contract Naming And Helper Grouping Pass

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: performed the narrow contract naming/helper
grouping pass for the publication/accessor package.

Changed files:

- `src/backend/prealloc/publication_plans.hpp`: added
  `prepared_storage_encoding_from_value_home_kind` as the clearer public helper
  name for the value-home-kind to storage-encoding contract.
- `src/backend/prealloc/publication_plans.cpp`: changed local scalar and
  store-source publication planning to call the clearer helper name, while
  keeping `prepared_publication_storage_encoding_from_home` as a compatibility
  adapter for existing public callers.

Deferred candidates:

- Broad public helper families in `prepared_lookups.hpp`,
  `decoded_home_storage.hpp`, `storage_plans.hpp`, and `value_locations.hpp`
  still have cross-package AArch64, x86, and test call sites. No rename there
  was local enough for this packet.
- `prepared_publication_storage_encoding_from_home` remains available because
  tests and existing publication-plan callers still depend on that public name;
  removing it would create broad caller churn outside the Step 3 boundary.

## Suggested Next

Execute Step 4 from `plan.md`: align prepared-printer mirrors only where the
real publication/accessor data-family naming changed. Since Step 3 did not
rename a data family or dump label, start by verifying that the prepared-printer
value-location and storage mirrors need no label/helper changes.

## Watchouts

- Keep the route behavior-preserving; no publication, storage, decoded-home,
  value-location, or prepared dump semantics changed in Step 3.
- Do not split `module.hpp`, `regalloc.hpp`, or broad aggregate prealloc
  contracts as part of naming cleanup.
- Keep compatibility comments or adapters when call sites still depend on the
  old aggregate prepared-module view.
- Avoid turning Step 4 into broad printer taxonomy churn unless a real
  publication/accessor data-family name changed.

## Proof

Ran delegated proof:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed; build completed and 162/162 backend tests passed.

Ran `git diff --check`: passed.

Proof log: `test_after.log`.
