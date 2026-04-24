Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Publish Coalescing Boundaries And Remaining Consumer Proof
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3.2 now publishes temporary-carrier provenance through direct step lookup
surfaces instead of making consumers read raw move-array positions:
`find_prepared_parallel_copy_move_for_step()` exposes the authoritative
carrier/source/destination record for a published `PreparedParallelCopyStep`,
the prepared printer now renders step detail through that helper, and the
cycle-bundle backend proofs now validate both control-flow and move-resolution
authority through published step lookup seams rather than insertion order.

## Suggested Next

Move to Step 3.3 and inspect whether any Step 3 consumers still depend on
implicit copy-coalescing or normalization behavior after the new step lookup
seams. Choose the smallest target-independent boundary that makes those
coalescing limits explicit in published bundle or consumer data, then keep the
proof focused on consumers following the published contract directly.

## Watchouts

- `find_prepared_parallel_copy_move_for_step()` is only honest while step
  ownership remains anchored to published `PreparedParallelCopyStep::move_index`
  facts; do not reintroduce consumer-side rescans over raw `moves`.
- Keep move-resolution step lookup unique per published step index so
  `find_prepared_out_of_ssa_parallel_copy_move_for_step()` continues to fail
  closed on ambiguous authority.
- Keep the route target-independent: any Step 3.3 coalescing boundary should
  publish through prepared bundle or consumer contract data, not x86-local
  placement shortcuts.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after adding the
published parallel-copy step-to-move helper, routing printer output through it,
and updating cycle-bundle proofs to use helper-based authority; canonical
proof log path is `test_after.log`.
