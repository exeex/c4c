Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Ordering, Cycle Breaking, And Carrier Use Truthful
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 3 now makes regalloc consume the published out-of-SSA phi parallel-copy
placement seam directly: `append_phi_move_resolution()` records phi
save/move/cycle-break resolution at the published execution block-entry bundle
site instead of recomputing join/block coordinates from execution-site
heuristics, and the focused liveness proof now checks that the authoritative
loop backedge move bundle preserves the published save/direct/cycle-temp order
at one shared placement.

## Suggested Next

Stay within Step 3 and audit the remaining downstream out-of-SSA consumers for
any residual carrier or ordering decisions that still reconstruct phi bundle
facts from positional coincidence instead of the published parallel-copy
bundle/move-bundle contract.

## Watchouts

- `append_phi_move_resolution()` now assumes published out-of-SSA phi bundle
  placement is block-entry at the bundle's published execution block. If a
  later route needs multiple authoritative phi bundles at one execution block,
  extend the published seam instead of restoring join-result scans or
  execution-site guessing.
- Keep the route target-independent: predecessor-owned, successor-entry, and
  critical-edge phi copies should continue to flow through published execution
  authority rather than backend-local placement rules.
- The strengthened loop-cycle proof now guards save/move/move ordering and
  cycle-temp sourcing directly through the authoritative move bundle; preserve
  that first-class bundle ordering if later packets touch cycle handling.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after switching
regalloc phi placement to the published out-of-SSA bundle seam and
strengthening the focused loop-cycle liveness proof; canonical proof log path
is `test_after.log`.
