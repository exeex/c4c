# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Consumers To The Authoritative Prepared Facts
Plan Review Counter: 5 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 slice for idea 62. The
`tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp` handoff
coverage now extends the pointer-backed same-module global guard-chain family
so it proves both that the x86 consumer keeps following authoritative prepared
addressing over drifted raw global carriers and that it rejects reopening raw
global fallback when the prepared addressing contract is removed from the
canonical guard-chain handoff.

## Suggested Next

Stay on the next bounded `plan.md` Step 3 short-circuit or remaining helper-
backed guard-family handoff packet that still lacks explicit contract proof for
missing or drifted prepared metadata, most likely another consumer lane where
prepared addressing or continuation ownership is authoritative but not yet
proven against raw-carrier drift and prepared-data loss.

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
- The minimal compare-branch routes now have contract-strict proof for zero,
  nonzero, and parameter-leaf variants, so the next Step 3 packet should move
  to a different consumer family instead of restating the same branch-lane
  proof shape.
- Keep the broader guard-chain and compare-join families aligned with the same
  prepared branch-condition and prepared-target contract; do not reopen raw
  compare or terminator recovery once the prepared control-flow block is
  authoritative.
- The joined-branch compare-join route intentionally continues to follow
  authoritative prepared entry labels over raw entry-label drift; do not
  convert that family to short-circuit-style label rejection, because the
  current migration proof already relies on authoritative prepared ownership
  winning over raw carrier labels there.
- The pointer-backed same-module global guard-chain lane now has explicit proof
  for prepared-address consumption and prepared-address-loss rejection, so the
  next packet should move to a different consumer family instead of restating
  that addressing contract.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after extending pointer-backed
same-module global guard-chain coverage so the route proves authoritative
prepared-address consumption and rejects raw global fallback after prepared
address loss. `test_after.log` is the proof artifact for this packet.
