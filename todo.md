Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Critical-Edge And Bundle Semantics
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 2 now also audits the next downstream x86 prepared-module consumer by
mutating the compare-join fixture from predecessor-owned execution to a
`successor_entry` handoff and proving the route still accepts the relocated
successor block-entry phi move bundle.

## Suggested Next

Deepen producer-side coverage on a fixture that truthfully publishes
`execution_site=successor_entry` from `out_of_ssa` itself instead of only
simulating the downstream handoff by mutating an already-prepared compare-join
bundle after regalloc.

## Watchouts

- The new audit proves the downstream successor-block consumer path, but the
  owned fixtures still do not exercise a real producer-published
  `execution_site=successor_entry` bundle.
- Keep this route target-independent; do not repair critical-edge handling with
  x86-local edge recovery or testcase-shaped splitting.
- Preserve idea 87's phi-elimination ownership while deepening bundle
  semantics.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Passed after the regalloc execution-site consumer change; proof log written to
`test_after.log`.
