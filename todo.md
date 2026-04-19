# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3.3.1
Current Step Title: Finish Immediate Materialized-Select Passthrough Surfaces
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3.3.1 slice for idea 62. The
`tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`
immediate selected-values and immediate selected-value-chain compare-join
routes now prove authoritative prepared entry-branch ownership ignores
non-compare entry-carrier drift for both the direct and `EdgeStoreSlot`
lanes.

## Suggested Next

Advance to `plan.md` Step 3.3.2 with one residual global-backed
materialized-select family at a time, starting with the narrowest remaining
same-module global compare-join route or helper gap rather than reopening the
now-complete immediate family.

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
- The immediate selected-values and selected-value-chain families now have
  route, passthrough-drift, return-context, and non-compare entry-carrier
  compare-join proof for both direct and `EdgeStoreSlot` lanes, so the next
  packet should move on to Step 3.3.2 instead of reopening Step 3.3.1.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after extending the immediate
materialized-select compare-join route coverage so the shared prepared
entry-branch handoff ignores non-compare entry-carrier drift for both direct
and `EdgeStoreSlot` lanes across the plain immediate and selected-value-chain
surfaces. `test_after.log` is the proof artifact for this packet.
