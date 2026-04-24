Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Ordering, Cycle Breaking, And Carrier Use Truthful
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 3 now proves the loop backedge bundle publishes its real cycle-break plan
directly: the prepared printer spells each published step with its ordered
source/destination action, cycle-temp use, and edge-store-slot carrier/storage
authority; the phi-materialize fixtures assert that same backedge bundle
surface directly; and the liveness proof now checks move-resolution lowering
against the published bundle ordering instead of backend-local
`move_resolution.reason` prefixes.

## Suggested Next

Stay within Step 3 and pick the next narrow consumer-facing seam that still
trusts implicit parallel-copy ordering or carrier facts. The best follow-on is
the first downstream reader that still reconstructs bundle sequencing or temp
use instead of consuming the published prepared bundle plan directly.

## Watchouts

- The loop backedge publication is explicitly `carrier=edge_store_slot` with
  per-move `storage=*.phi`; keep later packets honest about that published
  authority rather than simplifying it to carrier-free value swaps.
- Keep this route target-independent; do not repair critical-edge handling with
  x86-local edge recovery or testcase-shaped splitting.
- Preserve idea 87's phi-elimination ownership while deepening bundle
  semantics; consumer-facing helpers and dumps should read the published
  bundle plan directly instead of rebuilding ordering or carrier facts locally.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Passed after adding direct bundle-authority dump lines, deepening
loop-backedge step/carrier dump assertions, and replacing the loop-cycle
liveness dependency on `move_resolution.reason` prefixes with checks against
the published bundle plan; proof log written to `test_after.log`.
