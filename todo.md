Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Expose Bundle Authority In Dumps And Fixtures
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 2.3 now makes the Step 2.2 execution-block publication seam directly
visible in prepared output and focused fixtures: the prepared printer emits a
dedicated `authority execution_site=... execution_block=...` line for each
published parallel-copy bundle, the phi-materialize fixtures assert the
published execution block on predecessor-terminator bundles and the expected
absence on critical-edge bundles, and the authoritative-join / printer tests
prove both helper readers and dump readers stop exposing those labels once the
publication is removed instead of reconstructing CFG authority locally.

## Suggested Next

If Step 2 is accepted as complete, move into Step 3 with one narrow packet on
published ordering/cycle-breaking authority, most naturally around the
temporary-carrier path already exercised by the loop backedge bundle. If the
supervisor still sees a missing Step 2 family, keep the route target-
independent and extend only the first uncovered published bundle seam.

## Watchouts

- `out_of_ssa` still only builds `parallel_copy_bundles` from join-transfer
  edge transfers, so the dump/fixture deepening proves the published
  execution-block seam for existing families but does not by itself create a
  naturally-produced non-join `successor_entry` family.
- Keep this route target-independent; do not repair critical-edge handling with
  x86-local edge recovery or testcase-shaped splitting.
- Preserve idea 87's phi-elimination ownership while deepening bundle
  semantics; consumer-facing helpers and dumps should read the published
  execution block directly instead of rebuilding CFG authority locally.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Passed after adding direct bundle-authority dump lines, deepening
execution-block fixture assertions, and proving removed publication no longer
prints or helper-exposes reconstructed execution blocks; proof log written to
`test_after.log`.
