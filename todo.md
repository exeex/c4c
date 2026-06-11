Status: Active
Source Idea Path: ideas/open/174_route_index_facade_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Tighten Fail-Closed Typed Validation

# Current Packet

## Just Finished

Step 1 inventory selected Route 4 block-entry publication references as the
single typed category. Existing typed validation is
`bir::route4_validate_block_entry_publication_reference`, exposed through
`bir::route_index_validate_block_entry_publication_reference` on
`bir::RouteIndexReferenceFacade`; it already checks missing index/block/value,
stale owner, wrong key, wrong relationship, duplicate reference, and diverged
record cases without copying semantic payloads into facade records.

Selected consumer boundary: `mir::find_bir_block_entry_publication_identity`
in `src/backend/mir/query.cpp`. It is narrow because it already builds a
Route 4 publication index for the requested successor block and returns MIR
identity fields from the selected block-entry PHI record, rather than replacing
`PreparedFunctionLookups` or migrating AArch64 publication emission policy.

Oracle and proof subset for the selected path:
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` has focused Route 4
block-entry facade validation and prepared/BIR identity oracle checks;
`tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp` covers
the block-entry publication query boundary. `ctest --test-dir build -N`
confirmed the narrow test names `backend_prepared_lookup_helper` and
`backend_prealloc_block_entry_publications`.

## Suggested Next

Delegate Step 2 to tighten or complete fail-closed typed validation for the
selected Route 4 block-entry publication reference category, keeping
`Route4PublicationAvailabilityIndex` as the semantic owner and preserving
prepared/BIR identity checks as oracles.

## Watchouts

- Keep typed route records and indexes as the semantic owners.
- Do not turn `RouteIndexReferenceFacade` into a generic
  `PreparedFunctionLookups` replacement.
- Do not copy semantic payloads into facade records or scan broad BIR
  structures from consumers.
- Treat helper renames, expectation rewrites, or named-case-only validation as
  non-progress.
- Keep the Step 2 implementation scoped to Route 4 block-entry publication
  reference validation. Do not include current-block publication, Route 7
  comparison, AArch64 publication register selection, or prepared helper
  privatization in the same packet.

## Proof

Inventory-only packet; no build or ctest execution required and no proof logs
created. Discovery commands used targeted `rg`, file reads, c4c clang-tool
function-signature inventory, and `ctest --test-dir build -N` only.
