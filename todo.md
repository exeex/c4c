Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Ordering, Cycle Breaking, And Carrier Use Truthful
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Step 3 now publishes a shared prepared edge-to-bundle lookup seam for
branch-owned out-of-SSA parallel-copy bundles:
`find_published_parallel_copy_bundle_for_edge_transfer()` is the single
bundle-authority helper consumed by
`find_authoritative_branch_owned_parallel_copy_bundles()`, and the focused
backend proof now checks that the lookup fails once published edge ownership
is removed instead of silently recomputing bundle matches from CFG shape.

## Suggested Next

Step 3 has reached its `Plan Review Counter` limit. Before taking another
implementation packet on ordering/carrier authority, run plan review to decide
whether Step 3 should split into smaller numbered substeps or remain a single
route with a reset counter.

## Watchouts

- `find_published_parallel_copy_bundle_for_edge_transfer()` now treats invalid
  predecessor or successor labels as missing publication and must stay null in
  that state; do not let downstream readers reconstruct bundle ownership from
  branch CFG shape when the published edge labels are absent.
- Keep the route target-independent: predecessor-owned, successor-entry, and
  critical-edge phi copies should continue to flow through published execution
  authority rather than backend-local placement rules.
- The strengthened loop-cycle proof still guards save/move/move ordering and
  cycle-temp sourcing directly through the authoritative move bundle; preserve
  that first-class bundle ordering if later packets touch cycle handling.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after routing
branch-owned bundle lookup through the published edge-to-bundle seam and
strengthening authoritative join ownership coverage; canonical proof log path
is `test_after.log`.
