Status: Active
Source Idea Path: ideas/open/90_out_of_ssa_critical_edge_and_parallel_copy_deepening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Critical-Edge And Bundle Semantics
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 2 made regalloc consume published parallel-copy execution-site authority
instead of treating every bundle as predecessor-terminator executable by
default, and added backend coverage that locks predecessor-owned bundles to
their predecessor blocks while pinning branch-owned critical-edge bundles to
their published successor execution site.

## Suggested Next

Audit the next downstream consumer after regalloc for whether successor-entry
bundle authority is also consumed truthfully, and deepen coverage on a fixture
that actually publishes `execution_site=successor_entry` instead of only
predecessor-terminator and critical-edge cases.

## Watchouts

- The current packet proves predecessor-terminator and critical-edge consumer
  behavior, but the owned fixtures still do not exercise a real
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
