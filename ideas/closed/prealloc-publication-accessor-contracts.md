# Prealloc Publication Accessor Contracts

## Goal

Make the codegen-facing publication and prepared-accessor package in
`src/backend/prealloc` explicit without changing prepared-data semantics.

## Why This Exists

The responsibility map identifies `publication_plans.*`, `prepared_lookups.*`,
`decoded_home_storage.*`, `storage_plans.*`, and `value_locations.hpp` as a
first-class bridge between prepared facts and later consumers. This slice should
clarify that boundary before future target work adds more access patterns.

## Target Files

- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/decoded_home_storage.cpp`
- `src/backend/prealloc/decoded_home_storage.hpp`
- `src/backend/prealloc/storage_plans.cpp`
- `src/backend/prealloc/storage_plans.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `src/backend/prealloc/prepared_printer/storage.cpp`
- `src/backend/prealloc/prepared_printer/private.hpp`

## Slice Type

Header/data-contract contraction plus prepared-printer alignment where data
family names change.

## Durable Owner

Publication and prepared accessors.

## In Scope

- Clarify includes, names, and comments so publication/accessor files read as a
  deliberate API package.
- Remove or rewrite stale bridge/compatibility comments only when call sites
  prove the old boundary is gone.
- Keep printer labels and helper grouping aligned with any data-family naming
  changes made in this slice.
- Prefer small contract contraction over moving large amounts of code.

## Out Of Scope

- Changing publication semantics, decoded-home behavior, storage planning, or
  value-location meaning.
- Splitting `module.hpp` or `regalloc.hpp`.
- Broad codegen include churn that is not required by this package boundary.
- Printer-only taxonomy changes that do not follow data-contract changes.

## Expected Behavior-Preservation Proof

Run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Also run `git diff --check`.

## Acceptance Criteria

- The publication/accessor package is easier to identify from file names,
  comments, and includes.
- Any prepared-printer edits mirror real data-family naming changes.
- Backend tests remain green and prepared dump meaning is preserved.

## Reviewer Reject Signals

- Reject duplicate decode or home-selection logic introduced to justify a new
  contract shape.
- Reject broad codegen include churn when callers still depend on the same
  aggregate prepared-module view.
- Reject semantic changes to publication, storage, or value-location behavior
  claimed as layout cleanup.
- Reject deletion of compatibility comments unless the diff proves the old
  boundary no longer exists.
- Reject printer-only movement that invents a new ownership taxonomy not
  present in the data files.

## Closure Notes

Closed after Step 5 final boundary review.

The publication/accessor package is now easier to identify through localized,
behavior-preserving cleanup:

- `prepared_lookups.hpp` no longer carries an unused `<optional>` include.
- `value_locations.hpp` no longer uses stale step-fence, spelling-bridge, or
  legacy-caller wording for current contracts.
- `publication_plans.*` exposes the clearer helper name
  `prepared_storage_encoding_from_value_home_kind`.
- `prepared_publication_storage_encoding_from_home` remains as a compatibility
  adapter because existing cross-package callers still use that public surface.

Prepared-printer files required no edits because no prepared dump data-family
name, helper family, or output label changed. The work did not split
`module.hpp`, `regalloc.hpp`, or aggregate prealloc contracts, and it did not
change publication, storage, decoded-home, value-location, or prepared dump
semantics.
