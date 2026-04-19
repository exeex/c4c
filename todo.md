# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Consumers To The Authoritative Prepared Facts
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 slice for idea 62. The
`tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp` handoff
coverage now extends the short-circuit `EdgeStoreSlot` lane so it proves both
that the x86 consumer keeps following the authoritative prepared rhs
passthrough target over raw branch drift and that the shared short-circuit
branch-plan helper still publishes the canonical prepared labels after the join
carrier is rewritten to `PreparedJoinTransferKind::EdgeStoreSlot`.

## Suggested Next

Move to the next bounded `plan.md` Step 3 consumer family that still lacks an
explicit prepared-contract drift or loss proof, preferably outside the
short-circuit passthrough/helper lane that now has both select-carrier and
`EdgeStoreSlot` coverage.

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
- The short-circuit passthrough/helper lane now has explicit proof for both the
  select-carrier and `EdgeStoreSlot` forms, so the next packet should move to a
  different consumer family instead of restating the same continuation-target
  or branch-plan shape.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after extending short-circuit
`EdgeStoreSlot` coverage so the route proves authoritative rhs passthrough
target ownership over raw branch drift and preserves the helper-published
prepared branch-plan labels after the join carrier switches to
`PreparedJoinTransferKind::EdgeStoreSlot`. `test_after.log` is the proof
artifact for this packet.
