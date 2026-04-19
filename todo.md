# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Consumers To The Authoritative Prepared Facts
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 slice for idea 62. The
`tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp` handoff
coverage now extends the same-module global offset-store guard-chain lane so it
proves the x86 consumer rejects drifted prepared branch labels there instead of
reopening the raw guard-chain matcher, and the shared branch-label helper now
accepts the single-branch store route without assuming multi-branch-only guard
chains.

## Suggested Next

Move to the next bounded `plan.md` Step 3 consumer family that still lacks an
explicit prepared-contract drift or loss proof, preferably outside the
guard-chain lane now that the same-module global offset-store route has both
prepared-address loss and prepared-branch drift coverage.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- The guard-chain branch-label proof helper now covers both multi-branch and
  single-branch routes; keep future Step 3 additions aligned to authoritative
  prepared branch metadata instead of reintroducing raw matcher-specific
  helpers.
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
- The short-circuit passthrough/helper lane and the same-module global
  offset-store guard-chain lane both now have explicit contract drift coverage,
  so the next packet should move to a different consumer family instead of
  restating those shapes.

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
