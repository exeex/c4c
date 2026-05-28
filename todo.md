Status: Active
Source Idea Path: ideas/open/62_aarch64_shared_edge_dependency_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define Shared Edge Source Facts

# Current Packet

## Just Finished

Step 2 - Define Shared Edge Source Facts completed.

Added shared prepared edge-copy source fact APIs in
`src/backend/prealloc/prepared_lookups.hpp` and
`src/backend/prealloc/prepared_lookups.cpp`:

- `PreparedEdgeCopySourceFactsStatus` and
  `prepared_edge_copy_source_facts_status_name`.
- `PreparedEdgeCopySourceFacts`, carrying predecessor/successor/destination,
  publication, optional move, source value/home, destination home/storage,
  producer kind/block/instruction/pointers, and load-local memory-access facts.
- `prepare_edge_copy_source_facts(...)` for direct
  `(predecessor, successor, destination_value_id)` queries.
- `prepare_block_entry_parallel_copy_edge_source_facts(...)` for prepared
  block-entry parallel-copy move contexts.

The shared query now deliberately distinguishes missing prepared lookups,
invalid edge labels, missing destination values, missing publication, ambiguous
publication, unavailable publication, edge mismatch, unsupported move, move
edge mismatch, publication/move mismatch, missing source value, missing source
home, missing source producer, missing source memory access, and incomplete
source memory access. Focused coverage in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` validates available
named and immediate facts plus missing/ambiguous/fail-closed cases. No AArch64
edge-copy consumer migration files were changed.

## Suggested Next

Step 3 - Migrate Prepared Edge-Publication Consumers: route AArch64 edge-copy
consumers through the new shared prepared edge-copy source fact queries while
leaving target-local scratch, clobber, register spelling, and instruction
emission decisions in AArch64.

## Watchouts

`prepare_edge_copy_source_facts` treats named publications without a source
value id as `MissingSourceValue` before `MissingSourceHome`; a separate
coverage case validates `MissingSourceHome` when the id exists but the home is
absent. Producer kind `Unknown` remains a descriptive available fact rather
than a hard failure because register/stack-home edge copies can be emitted from
home facts without materializing a producer. Step 3 should decide which AArch64
callers require producer-specific materialization and fail closed on the
returned status instead of reviving predecessor-depth scans.

## Proof

`(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Proof passed: 169/169 backend tests passed. Log path: `test_after.log`.
