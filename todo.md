# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3.3.1
Current Step Title: Finish Immediate Materialized-Select Passthrough Surfaces
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Plan review split former `plan.md` Step 3.3 into numbered substeps after the
review threshold was reached. The latest completed executor packet advanced the
new Step 3.3.1 surface for idea 62. The
`tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`
plain immediate selected-values joined-branch route-level handoff now proves
authoritative prepared ownership survives true-lane and false-lane
passthrough topology drift for both the direct and `EdgeStoreSlot` lanes.

## Suggested Next

Continue `plan.md` Step 3.3.1 by adding the adjacent plain immediate
selected-values compare-join helper passthrough-drift proof that still lacks
explicit true-lane and false-lane coverage, while keeping the packet bounded
to that immediate family instead of widening into global-backed variants.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- Keep `PreparedBranchCondition` and `PreparedControlFlowBlock` targets
  contract-consistent; mismatches should still fail the canonical
  prepared-module handoff instead of silently preferring whichever record
  happens to look usable locally.
- Keep Step 3 packets focused on consumer migration proof, not on reopening
  Step 2.3-style fallback cleanup that already landed for stricter handoff
  surfaces.
- The rewritten Step 3.3 route now starts with immediate materialized-select
  surfaces, so keep the next packet inside Step 3.3.1 until the remaining
  immediate compare-join helper drift proof is closed.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after extending the plain
immediate selected-values joined-branch route-level coverage so the shared
prepared handoff ignores true-lane and false-lane passthrough topology drift
for both direct and `EdgeStoreSlot` lanes. `test_after.log` is the proof
artifact for this packet.
