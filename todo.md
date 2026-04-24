Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Critical-Edge And Bundle Semantics
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 2 now also audits the next downstream x86 prepared-module consumer by
mutating the compare-join fixture from predecessor-owned execution to a
`successor_entry` handoff and proving the route still accepts the relocated
successor block-entry phi move bundle.

## Suggested Next

Confirm whether Step 2 needs a producer-path extension for truthful
`execution_site=successor_entry` publication or whether the current
`join_transfers -> parallel_copy_bundles` pipeline makes that shape
unreachable, then shift the next packet to another owned publication seam if
successor-entry bundles remain consumer-only audit territory.

## Watchouts

- The new audit proves the downstream successor-block consumer path, but
  `parallel_copy_bundles` are currently built only from `join_transfers`,
  which appear to require multi-predecessor join blocks and therefore may make
  producer-published `execution_site=successor_entry` bundles unreachable.
- Keep this route target-independent; do not repair critical-edge handling with
  x86-local edge recovery or testcase-shaped splitting.
- Preserve idea 87's phi-elimination ownership while deepening bundle
  semantics.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Passed after the regalloc execution-site consumer change; proof log written to
`test_after.log`.
