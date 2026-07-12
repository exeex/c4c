# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.4
Current Step Title: Migrate floating-point and general value consumers

## Just Finished

- Plan Step 2.4 removed executable same-block named-producer discovery from
  `fp_value_materialization.cpp`. Floating-point/general value materialization
  now consumes the traversal-attached common producer authority, fails closed
  when that authority is unavailable, and preserves existing instruction and
  ABI policy.

## Suggested Next

- Execute Plan Step 2.5 as a bounded packet against `memory.cpp`, migrating
  target-local lookup rebuilding to traversal-attached common authority.

## Watchouts

- Step 2.4 required no test changes or common contract changes. Producer
  instruction identity and indices now come from the same attached query;
  preserve that fail-closed boundary in later consumer families. The prior
  scalability timeout and selected-global-load fused-branch stale-stack-home
  failure remain supervisor-level broader-proof watchouts.

## Proof

- Passed the exact supervisor-selected proof: `cmake --build --preset default`
  followed by the three-test CTest subset for HFA result-home publication,
  current-block direct publication identity, and pointer-value named scalar
  writeback (3/3). Canonical proof output is in `test_after.log`; the subset
  was sufficient for this bounded Step 2.4 consumer migration.
