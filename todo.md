Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Ordering, Cycle Breaking, And Carrier Use Truthful
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Step 3 now publishes a shared prepared execution-block lookup seam for
out-of-SSA parallel-copy bundles: `published_prepared_parallel_copy_execution_block_index()`
is the single block-index authority consumed by both
`find_prepared_out_of_ssa_parallel_copy_move_bundle()` and
`append_phi_move_resolution()`, and the focused backend proof now checks that
predecessor-owned bundles resolve stable execution indices while critical-edge
bundles keep the lookup empty.

## Suggested Next

Stay within Step 3 and audit the remaining downstream prepared helpers for any
bundle-authority lookup that still rescans raw `parallel_copy_bundles`
instead of flowing through one first-class publication seam, especially around
branch-owned join bundle helpers.

## Watchouts

- `published_prepared_parallel_copy_execution_block_index()` is intentionally
  empty for `critical_edge` bundles with no published execution block. Preserve
  that `std::nullopt` contract unless the route explicitly publishes a
  different execution block for those bundles.
- Keep the route target-independent: predecessor-owned, successor-entry, and
  critical-edge phi copies should continue to flow through published execution
  authority rather than backend-local placement rules.
- The strengthened loop-cycle proof still guards save/move/move ordering and
  cycle-temp sourcing directly through the authoritative move bundle; preserve
  that first-class bundle ordering if later packets touch cycle handling.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after switching
regalloc phi placement to the published out-of-SSA bundle seam and
strengthening the focused loop-cycle liveness proof; canonical proof log path
is `test_after.log`.
