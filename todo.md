Status: Active
Source Idea Path: ideas/open/64_aarch64_join_parallel_copy_prepared_query.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Migrate AArch64 Join-Copy Consumers To The Shared Query

# Current Packet

## Just Finished

Step 3 completed: AArch64 current-block join consumers now use
`prepare_current_block_join_parallel_copy_source_facts` instead of rebuilding
source relationships from raw block-entry move bundles.

`dispatch_producers.cpp` now keeps only a target-local adapter/cache for
instruction-index routing:
- it builds missing value-home and edge-publication lookup inputs through
  shared prepared lookup builders when a test or caller has not attached
  `PreparedFunctionLookups`;
- it calls the shared query once per block in
  `build_current_block_join_parallel_copy_cache`;
- it indexes the query's incoming-expression and source-value id sets to fill
  the existing per-instruction boolean routing cache; and
- invalid cache/context fallback now fails closed instead of running the old
  local reconstruction helpers.

`is_current_block_join_parallel_copy_source` also asks the shared query and
checks the prepared source-value set, so
`dispatch_value_materialization.cpp` no longer reaches a raw-bundle semantic
path through that helper. The old same-block dependency walk, local
incoming-expression scanner, and local source scanner were removed from
`dispatch_producers.cpp`.

Focused AArch64 fixture updates added explicit prepared join-transfer and
parallel-copy metadata where tests previously encoded the expected authority
only as raw move bundles. That keeps the tested behavior intact while requiring
the new shared source authority.

## Suggested Next

Step 4 should retire or rename the remaining AArch64 cache surface so it is
clearly a routing adapter, not source authority. Consider replacing the public
`build_current_block_join_parallel_copy_cache`/`cached_*` names with names that
mention prepared-query routing, and remove any now-unnecessary public exposure
once direct consumers can use the shared query or adapter intentionally.

## Watchouts

Do not reintroduce raw move-bundle scanning while retiring the cache naming.
The adapter may synthesize missing lookup indexes through shared prepared
builders, but source relationships must continue to come from
`prepare_current_block_join_parallel_copy_source_facts`. Keep branch-fusion
sequencing, before-instruction filtering, before-return publication ordering,
edge fallback, and select-chain dependency discovery unchanged.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications|backend_prepare_authoritative_join_ownership)$') > test_after.log 2>&1`

`test_after.log` contains the green build and focused CTest run.
`git diff --check` also passed.
