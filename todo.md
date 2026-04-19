# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Consumers To The Authoritative Prepared Facts
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed a `plan.md` Step 3 slice for idea 62. The residual local-slot
guard lane boundary coverage in
`tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp`
now proves that the add-chain, sub-guard, and byte-addressed local-slot
consumers keep following authoritative prepared branch ownership even after
their raw entry compare and true/false carrier labels are drifted away from
the canonical prepared handoff.

## Suggested Next

Advance another bounded `plan.md` Step 3 consumer family that still reads raw
branch topology in x86 codegen, preferably the remaining local-slot or joined
branch helpers that lack a prepared-ownership-over-drift proof comparable to
the local-slot guard lane.

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
  Step 2.3-style fallback cleanup that already landed for local-slot and
  countdown handoff surfaces.
- The new local-slot drift checks only mutate existing entry compare and
  branch-label carriers; avoid introducing extra blocks when proving prepared
  ownership for shape-sensitive consumers whose bounded matcher still depends
  on the original function block count.
- Prefer prepared metadata drift proofs over widening the specialized x86
  matcher surface with new testcase-shaped topology cases.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after adding local-slot guard-lane
coverage that drifts the raw entry compare and true/false labels while
preserving prepared branch facts, and verifies the x86 prepared-module
consumer still emits the canonical asm from authoritative prepared ownership.
`test_after.log` is the proof artifact for this packet.
