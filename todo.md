Status: Active
Source Idea Path: ideas/open/695_bir_route_facade_named_compatibility_adapters.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The First Facade Consumer

# Current Packet

## Just Finished

Completed `plan.md` Step 1 discovery. Direct facade users are:
`src/backend/bir/bir.cpp` Route 7 fused-compare/materialized-condition
helpers, `src/backend/mir/aarch64/codegen/comparison.cpp` Route 7 comparison
diagnostic validation, `src/backend/mir/query.cpp` Route 4 block-entry
publication validation, `src/backend/prealloc/prepared_lookups.cpp` Route 4
block-entry attribution, plus focused test helpers.

First consumer selected:
`src/backend/prealloc/prepared_lookups.cpp`
`attribute_route4_block_entry_publication_if_agreeing`.

Current facade entry point:
`bir::route_index_reference_facade(route4_publications)` followed by
`bir::route_index_validate_block_entry_publication_reference(route4_facade,
route4_successor, *query.route4_destination_value)`.

Intended named adapter:
`bir::validate_block_entry_publication_reference` or equivalent
publication-owned adapter name that accepts the Route 4 publication index
directly and delegates to the existing Route 4 block-entry validation path
during compatibility migration.

Rollback point:
restore the local `route4_facade` construction plus
`route_index_validate_block_entry_publication_reference` call inside
`attribute_route4_block_entry_publication_if_agreeing`; no caller contract,
prepared query input, result field, expectation, allowlist, timeout, or
runtime behavior rollback should be needed.

Why this can move behavior-preservingly:
the call site only copies `Route4IndexReferenceValidation::status` and
`route_status` into attribution metadata, then keeps the existing
`route4_block_entry_publication_record` cross-check before setting
`route4_block_entry_publication_attributed`. A named publication adapter can
return the same validation record and statuses while preserving the current
missing-record, duplicate, mismatch, stale-owner, and instruction-index
behavior. It does not require expectation rewrites, unsupported-marker edits,
allowlist changes, timeout changes, executable behavior changes, or default
harness-contract changes.

## Suggested Next

Implement Step 2 for the selected Route 4 publication surface: add the named
block-entry publication validation adapter in `src/backend/bir/bir_route_index.hpp`
and `src/backend/bir/bir_route_facade.cpp` as a thin compatibility delegate to
the existing Route 4 validation path, then migrate only
`attribute_route4_block_entry_publication_if_agreeing` in
`src/backend/prealloc/prepared_lookups.cpp` if the supervisor includes Step 3
in the same packet.

## Watchouts

Keep the adapter publication-owned, not route-index-owned, even if it delegates
to Route 4 compatibility records internally. Do not migrate the separate
`src/backend/mir/query.cpp` Route 4 consumer or any Route 7 consumer in the
same packet unless the supervisor explicitly widens scope. Do not change
prepared authority, target lowering, Route 5 publication cleanup, stack
authority, dump vocabulary, expectations, unsupported markers, allowlists,
timeouts, or runtime contracts.

## Proof

Discovery proof run:
`rg -n "RouteIndexReferenceFacade|route_index_reference_facade|route_index_validate_" src/backend tests/backend`

No build was required for this discovery-only packet. Per the delegated
do-not-touch list, no root-level `.log` proof file was created.

Recommended focused implementation proof for the next packet:
`cmake --build --preset default --target backend_prealloc_block_entry_publications_test && ctest --test-dir build -R '^backend_prealloc_block_entry_publications$' --output-on-failure`
