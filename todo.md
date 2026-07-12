# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Migrate select and comparison consumers

## Just Finished

- Plan Step 2.3 removed both local `PreparedValueHomeLookups` reconstruction
  sites from select materialization. Select/comparison consumers now use only
  traversal-attached common value-home authority and fail closed when it is
  absent; comparison/select instruction policy was unchanged.

## Suggested Next

- Execute Plan Step 2.4 as a bounded packet against
  `fp_value_materialization.cpp`, migrating named-producer discovery to the
  existing attached common query.

## Watchouts

- Step 2.3 required no test changes or common contract changes. Preserve its
  fail-closed attached-authority boundary when working on later consumer
  families. The prior scalability timeout and selected-global-load
  fused-branch stale-stack-home failure remain supervisor-level broader-proof
  watchouts.

## Proof

- Passed the exact supervisor-selected proof: `cmake --build --preset default`
  followed by the four-test CTest subset for pointer-select aggregate copy,
  branch comparison, branch control, and current-block join routing (4/4).
  Canonical proof output is in `test_after.log`; the subset was sufficient for
  this bounded Step 2.3 consumer migration.
