Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Critical-Edge And Bundle Semantics
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Step 2 settled the producer-side `execution_site=successor_entry` question in
`out_of_ssa`: `parallel_copy_bundles` are still published only from
`join_transfers`, so the producer now truthfully classifies those bundles as
either `predecessor_terminator` or `critical_edge`, and the prepare-level
joined-branch proof now covers the select-materialized producer path without
claiming a reachable successor-entry publication shape.

## Suggested Next

Treat any remaining Step 2 `successor_entry` work as downstream consumer-only
audit coverage and shift the next producer packet to another reachable
publication seam, such as strengthening the printed or lookup contract around
the existing `predecessor_terminator` and `critical_edge` bundle ownership.

## Watchouts

- `out_of_ssa` still does not publish non-join parallel-copy bundles; if a
  future Step 2 route really needs producer-published `successor_entry`, it
  will require a new publication path instead of more join-transfer
  reinterpretation.
- Keep this route target-independent; do not repair critical-edge handling with
  x86-local edge recovery or testcase-shaped splitting.
- Preserve idea 87's phi-elimination ownership while deepening bundle
  semantics.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Passed after the producer-side classification change and prepare-level bundle
coverage update; proof log written to `test_after.log`.
