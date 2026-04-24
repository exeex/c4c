Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Ordering, Cycle Breaking, And Carrier Use Truthful
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 3 now publishes an authority-aware prepared move-bundle lookup seam for
out-of-SSA block-entry consumers: `prealloc.hpp` resolves the prepared
move-bundle directly from the published parallel-copy bundle execution
authority, and the focused compare-branch, loop-countdown, short-circuit, and
liveness proofs consume that seam instead of reconstructing bundle ownership
from block position alone.

## Suggested Next

Stay within Step 3 and move to the next downstream out-of-SSA consumer that
still re-derives ordering, cycle-break, or carrier/storage facts from
block/instruction coordinates instead of the published parallel-copy contract.

## Watchouts

- The new lookup helper is intentionally limited to out-of-SSA block-entry
  bundles with published parallel-copy execution authority; do not widen later
  callers back toward raw block-index scans when the bundle authority is
  already available.
- Preserve the existing target-independent execution-site contract:
  predecessor, successor-entry, and critical-edge placement should continue to
  flow from published bundle authority, not x86-local reconstruction.
- If a later consumer needs to distinguish multiple authority bundles at the
  same execution block, extend the published seam rather than falling back to
  ad hoc positional inference.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`
Passed. Backend build and the `^backend_` subset stayed green after publishing
the authority-aware out-of-SSA move-bundle lookup helper and updating the
focused compare-branch, loop-countdown, short-circuit, and liveness proofs;
proof log written to `test_after.log`.
