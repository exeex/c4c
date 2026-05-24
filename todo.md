Status: Active
Source Idea Path: ideas/open/prealloc-publication-accessor-contracts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Align Prepared Printer Mirrors

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: verified the prepared-printer mirrors after
the Step 2/3 publication/accessor cleanup.

Decision:

- No prepared-printer edit was required.
- Step 2 changed stale wording and an unused include in publication/accessor
  headers, not a prepared dump data-family name or label.
- Step 3 added `prepared_storage_encoding_from_value_home_kind` as a clearer
  helper for the value-home-kind to storage-encoding contract, while preserving
  `prepared_publication_storage_encoding_from_home` as a compatibility adapter.
  That helper rename does not change the value-location or storage-plan data
  family names that the prepared printer mirrors.

Checked mirrors:

- `src/backend/prealloc/prepared_printer/value_locations.cpp` still prints the
  unchanged `--- prepared-value-locations ---` section and value-home fields.
- `src/backend/prealloc/prepared_printer/storage.cpp` still prints the
  unchanged `--- prepared-storage-plans ---` section and storage-encoding
  fields.
- `src/backend/prealloc/prepared_printer/private.hpp` still exposes the same
  append helpers for those data families.

## Suggested Next

Execute Step 5 from `plan.md`: perform the final boundary review, confirm the
diff remains behavior-preserving, and decide whether
`ideas/open/prealloc-publication-accessor-contracts.md` is ready for lifecycle
closure.

## Watchouts

- Keep the route behavior-preserving; no publication, storage, decoded-home,
  value-location, or prepared dump semantics changed in Step 3.
- Do not split `module.hpp`, `regalloc.hpp`, or broad aggregate prealloc
  contracts as part of naming cleanup.
- Step 4 deliberately made no printer taxonomy change because there was no
  corresponding data-family rename or dump-label change to mirror.
- `prepared_publication_storage_encoding_from_home` remains as a compatibility
  adapter for existing cross-package callers.

## Proof

Ran delegated proof for this metadata-only packet:

`git diff --check`

Result: passed.
