Status: Active
Source Idea Path: ideas/open/prealloc-publication-accessor-contracts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Boundary Review

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: performed the final boundary review for the
prealloc publication/accessor contracts plan.

Final package-boundary summary:

- The active package remains the prepared-publication API surface:
  `publication_plans.*`, `prepared_lookups.*`, `decoded_home_storage.*`,
  `storage_plans.*`, and `value_locations.hpp`.
- Step 1 recorded the public declaration, include, stale-wording, and
  prepared-printer mirror map without changing implementation files.
- Step 2 tightened the boundary by removing an unused `<optional>` include from
  `prepared_lookups.hpp` and replacing stale `value_locations.hpp` wording
  around step fences, spelling bridges, and legacy callers.
- Step 3 added the clearer helper name
  `prepared_storage_encoding_from_value_home_kind` in
  `publication_plans.*`, preserving
  `prepared_publication_storage_encoding_from_home` as a compatibility adapter
  for existing cross-package callers.
- Step 4 verified that no prepared-printer edit was needed because no prepared
  dump data-family name, helper family, or section label changed.

Changed files across this run:

- `plan.md`
- `todo.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`

Boundary confirmations:

- No publication, storage, decoded-home, value-location, or prepared dump
  semantics changed.
- No duplicate decode, home-selection, or storage-selection logic was
  introduced.
- No broad codegen include churn occurred.
- No `module.hpp`, `regalloc.hpp`, or aggregate prealloc contract split was
  performed.
- No prepared-printer taxonomy was invented independent of the data contracts.

## Suggested Next

Ask the plan owner to close
`ideas/open/prealloc-publication-accessor-contracts.md`. The source idea's
acceptance criteria are satisfied: the package boundary is easier to identify,
prepared dump meaning is preserved, and backend proof has remained green on the
code-changing packets.

## Watchouts

- The compatibility adapter
  `prepared_publication_storage_encoding_from_home` intentionally remains for
  existing cross-package callers; removing it would be a separate caller-churn
  slice.
- Broader public helper renames in lookup, decoded-home, storage-plan, and
  value-location surfaces were deferred because they would cross package
  boundaries.

## Proof

Delegated proof for this metadata-only final review:

`git diff --check`

Result: passed.

Prior code-changing packets in this run passed:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`
