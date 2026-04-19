# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3.3.1
Current Step Title: Finish Immediate Materialized-Select Passthrough Surfaces
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.3.1 slice for idea 62. The
`tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`
plain immediate selected-values compare-join helper now proves authoritative
prepared return-context ownership survives true-lane and false-lane
passthrough topology drift for both the direct and `EdgeStoreSlot` lanes.

## Suggested Next

Continue `plan.md` Step 3.3.1 by closing the next adjacent immediate-family
passthrough-drift gap, keeping the packet inside the remaining immediate
materialized-select surfaces rather than widening into global-backed or
trailing-join variants.

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
- The plain immediate selected-values family now has both route-level and
  compare-join helper passthrough-drift proof, so the next packet should stay
  within any residual immediate Step 3.3.1 surface rather than reopening this
  family.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after extending the plain
immediate selected-values compare-join helper coverage so the shared prepared
return-context handoff ignores true-lane and false-lane passthrough topology
drift for both direct and `EdgeStoreSlot` lanes. `test_after.log` is the
proof artifact for this packet.
