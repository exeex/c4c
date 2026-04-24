Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Critical-Edge And Bundle Semantics
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Step 2 now publishes a direct lookup surface for existing branch-owned
parallel-copy bundle authority: `prealloc.hpp` gained
`find_authoritative_branch_owned_parallel_copy_bundles(...)`, which reuses the
published authoritative join-transfer edges and returns the corresponding
`true_bundle` / `false_bundle` without forcing downstream readers to rescan
`parallel_copy_bundles` manually. The prepare-level authoritative-ownership
test now proves both a purely `predecessor_terminator` branch join and the
mixed `critical_edge` + `predecessor_terminator` short-circuit case through
that shared helper.

## Suggested Next

Step 2 has reached its review threshold. Before taking another Step 2 packet,
review whether `Publish Critical-Edge And Bundle Semantics` should split into
numbered substeps, or whether the next honest packet should advance directly to
Step 3 ordering / carrier publication using the now-explicit bundle lookup
surface.

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
Passed after adding the branch-owned bundle lookup helper and prepare-level
authority coverage; proof log written to `test_after.log`.
