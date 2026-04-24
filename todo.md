Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Publish Coalescing Boundaries And Remaining Consumer Proof
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3.3 now publishes the remaining regalloc-side coalescing boundary for
out-of-SSA parallel-copy consumers: authoritative move-resolution records keep
their published `source_parallel_copy_step_index` even when assigned storage
coalesces the transfer, and those records now mark
`coalesced_by_assigned_storage` explicitly instead of disappearing behind
consumer-local storage-equality inference. The phi-join backend proof now
checks the right-edge consumer seam through published bundle step lookup and
verifies that the coalesced incoming edge is still represented by explicit
out-of-SSA authority.

## Suggested Next

Move to Step 4 closure review: decide whether idea 90 is now complete or
whether any remaining consumer still depends on unpublished edge-copy
authority outside the Step 3 surfaces.

## Watchouts

- `coalesced_by_assigned_storage` is a publication bit for out-of-SSA consumer
  authority, not permission to revive consumer-local coalescing heuristics.
- Keep `find_prepared_out_of_ssa_parallel_copy_move_for_step()` fail-closed on
  duplicate published step authority; Step 3.3 depends on one explicit record
  per published step whether the transfer survives as a real move or as a
  coalesced boundary.
- The new publication is intentionally scoped to Step 3 out-of-SSA move
  bundles; do not generalize it into unrelated call/return move-resolution
  paths without a separate contract decision.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after publishing
explicit coalesced-step authority in out-of-SSA move resolution and updating
the phi-join consumer proof to use published bundle step lookup directly;
canonical proof log path is `test_after.log`.
