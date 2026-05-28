Status: Active
Source Idea Path: ideas/open/64_aarch64_join_parallel_copy_prepared_query.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define The Shared Prepared Join-Copy Query

# Current Packet

## Just Finished

Step 2 completed: shared prepared code now exposes
`prepare_current_block_join_parallel_copy_source_facts` through
`src/backend/prealloc/prepared_lookups.hpp/.cpp`.

The query takes prepared names, value locations, optional value-home lookups,
edge-publication lookups, the current BIR block, and the current successor
label. It returns:
- top-level query status for missing names, value locations, edge-publication
  lookups, block, or successor label;
- per-move source facts tied to the original `PreparedMoveBundle`,
  `PreparedMoveResolution`, and `PreparedEdgePublication`;
- source and destination value ids/names, homes, home kinds, storage kind, and
  immediate-source classification;
- explicit per-move `PreparedEdgeCopySourceFactsStatus` for missing,
  ambiguous, unsupported, or mismatched shared authority;
- derived incoming-expression value ids/names, including same-block
  `Binary`/`Cast`/`Select` named-operand closure; and
- derived source value ids/names for register destinations, immediate
  destinations, shared-register sources, and stack-homed sources.

Focused coverage was added to
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for available
named, immediate, stack-source, unsupported-move, and missing
edge-publication-lookup cases. No AArch64 dispatch consumers were migrated.

## Suggested Next

Step 3 should migrate AArch64 current-block join consumers to call
`prepare_current_block_join_parallel_copy_source_facts` once per current block
and use its prepared sets/statuses instead of rebuilding the cache from raw
move bundles. Keep the first migration narrow: replace the incoming-expression
and source-result routing checks while preserving existing fallback behavior
only as explicit fail-closed handling, not as a second semantic authority.

## Watchouts

The new query intentionally requires shared edge-publication lookups for
source authority; Step 3 should not recover by scanning raw bundles locally
when the query reports missing or ambiguous publication facts. The same-block
expression closure still walks BIR `Binary`/`Cast`/`Select` operands, so
AArch64 migration must preserve instruction-index routing separately and avoid
changing branch-fusion sequencing, before-instruction filtering,
before-return publication ordering, edge fallback, or select-chain dependency
discovery.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_prealloc_block_entry_publications|backend_prepare_authoritative_join_ownership)$') > test_after.log 2>&1`

`test_after.log` contains the green build and focused CTest run.
`git diff --check` also passed.
