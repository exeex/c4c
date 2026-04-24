Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Publish Remaining Edge-Owned Bundle Authority
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 2.2 now publishes direct execution-block ownership on
`PreparedParallelCopyBundle`: `out_of_ssa` records the authoritative block
that executes each published edge-owned bundle, `prealloc.hpp` exposes that
seam through `published_prepared_parallel_copy_execution_block_label(...)`,
and the compare-join handoff proof uses that published block directly when
relocating a bundle to `successor_entry` instead of inferring ownership from
raw predecessor/successor CFG shape. Focused prepare and printer tests now
cover the new execution-block authority and its dump surface.

## Suggested Next

Stay within Step 2.2 only if the route still needs a naturally-produced
non-join `successor_entry` family from `out_of_ssa`; otherwise the next
coherent packet is Step 2.3 dump/fixture deepening around the new
execution-block publication seam.

## Watchouts

- `out_of_ssa` still only builds `parallel_copy_bundles` from join-transfer
  edge transfers, so the new execution-block seam makes edge ownership direct
  for published bundles but does not by itself create a naturally-produced
  non-join `successor_entry` family.
- Keep this route target-independent; do not repair critical-edge handling with
  x86-local edge recovery or testcase-shaped splitting.
- Preserve idea 87's phi-elimination ownership while deepening bundle
  semantics; consumer-facing helpers should read the new publication instead of
  rebuilding CFG authority locally.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Passed after publishing bundle execution-block ownership, updating the
consumer-facing successor-entry handoff test, and refreshing dump/fixture
expectations; proof log written to `test_after.log`.
