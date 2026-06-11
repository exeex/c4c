Status: Active
Source Idea Path: ideas/open/204_route5_edge_join_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Fail-Closed Route 5 Reader Adapter

# Current Packet

## Just Finished

Step 2 added the fail-closed Route 5 current-block join-source reader adapter
for `build_current_block_join_prepared_query_routing`.

`mir::find_bir_current_block_join_source_identity` now validates indexed
Route 5 current-block join records against the successor block, PHI
predecessor label/id, destination value identity/type, source value
identity/type, destination PHI/instruction/index, and named source producer
instruction/index before reporting an available identity. Empty, duplicate,
wrong-predecessor, mismatched, incomplete, no-source, and missing-producer
Route 5 cases remain unavailable so the selected AArch64 routing boundary falls
back to prepared query behavior.

Prepared state remains authoritative for scheduling, storage homes, move
bundles, branch policy, wrappers, final edge-copy records, coalescing,
execution-site placement, and emitted output. The adapter only feeds the
incoming-expression/source routing bitsets with validated Route 5 semantic
identity.

## Suggested Next

Execute Step 3 for the selected boundary: review whether any remaining wiring
is needed at `build_current_block_join_prepared_query_routing`, then keep
prepared move scheduling, homes, wrappers, branch policy, final edge-copy
records, and output authority unchanged while expanding only focused success
and fallback proof if the supervisor wants another implementation slice.

## Watchouts

- Route 5 is semantic identity only; prepared state remains authoritative for
  scheduling, storage, wrappers, branch policy, and output.
- The indexed Route 5 adapter deliberately rejects duplicate exact
  join-source records and wrong-predecessor records even when destination and
  source values otherwise match.
- Memory-like named sources can identify join-source expressions, but they do
  not become scheduling or storage authority.
- Do not claim progress through expected-output weakening, unsupported
  downgrades, helper renames, or testcase-shaped matching.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed. Proof log path: `test_after.log`.
