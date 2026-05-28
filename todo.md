Status: Active
Source Idea Path: ideas/open/64_aarch64_join_parallel_copy_prepared_query.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Retire The Local Cache Authority

# Current Packet

## Just Finished

Step 4 completed: the old AArch64 join-copy cache authority surface was renamed
to prepared-query routing terminology.

Renamed public surface:
- `CurrentBlockJoinParallelCopyCache` ->
  `CurrentBlockJoinPreparedQueryRouting`
- `build_current_block_join_parallel_copy_cache` ->
  `build_current_block_join_prepared_query_routing`
- `cached_current_block_join_parallel_copy_incoming_expression` ->
  `current_block_join_prepared_query_incoming_expression`
- `cached_current_block_join_parallel_copy_source` ->
  `current_block_join_prepared_query_source`
- `is_current_block_join_parallel_copy_source` ->
  `prepared_query_current_block_join_parallel_copy_source`

The implementation remains a target-local instruction-index routing adapter
over `prepare_current_block_join_parallel_copy_source_facts`; it does not scan
raw prepared homes or move bundles for source relationships. `dispatch.cpp`
and `dispatch_value_materialization.cpp` now call the prepared-query routing
names directly.

## Suggested Next

Step 5 should prove join-copy query equivalence and closure readiness. Re-run
the focused AArch64 and shared prepared lookup subset, scan for the retired
cache names and raw reconstruction patterns, and consider whether the
remaining adapter should stay public or be made private after proof.

## Watchouts

Keep Step 5 proof focused on behavior equivalence. The old cache names should
stay retired, and any remaining local AArch64 logic should remain limited to
instruction-index routing over shared prepared-query facts. Do not broaden into
branch-fusion sequencing, before-instruction filtering, before-return
publication ordering, edge fallback, or select-chain dependency discovery.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications|backend_prepare_authoritative_join_ownership)$') > test_after.log 2>&1`

`test_after.log` contains the green build and focused CTest run.
`git diff --check` also passed.
