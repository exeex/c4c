# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3.4
Current Step Title: Loop-Carry And Residual Consumer Cleanup
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening `render_minimal_compare_branch_if_supported()` so the plain
param-zero branch lane now rejects drifted direct-return leaf rewrites once
authoritative prepared join ownership is still published for the entry block,
instead of accepting a fallback past the compare-join/join handoff, and by
adding regressions for both the select-carried and `EdgeStoreSlot` joined
branch variants that mutate the prepared fixture into direct return leaves
after prepare.

## Suggested Next

Stay in Step 3.4 and inspect the remaining plain param-zero branch and joined
consumer boundaries for any other route where authoritative prepared branch or
join ownership can still be bypassed by a later generic guard/countdown helper
after post-prepare module drift.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not widen into
  Step 3.2 compare-join reproving, Step 3.3 carrier hunting, Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- The local-slot short-circuit rhs continuation must now reject once a prepared
  rhs branch condition exists but no longer yields a valid prepared compare
  context; do not reopen the raw trailing-compare fallback behind that
  authoritative contract.
- The local-slot short-circuit entry branch must now reject once authoritative
  branch-owned join ownership exists but the entry prepared branch contract has
  drifted; do not let that route fall through into the generic plain-condition
  lane.
- The param-zero compare-join and joined-branch lane must now reject once
  authoritative branch-owned join ownership exists but the resolved prepared
  compare-join render contract disappears; do not let that route silently drop
  past the compare-join handoff into any adjacent generic branch family.
- The plain param-zero branch lane must now also reject if the module is
  post-prepare mutated into direct-return leaves while authoritative joined
  prepared ownership for the same entry block remains active; do not reopen
  that fallback while checking adjacent residual helpers.
- The local countdown fallback must stay unavailable once a function carries
  authoritative `PreparedJoinTransferKind::LoopCarry` ownership; do not reopen
  that bypass while chasing other residual helpers.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3.4 handoff proof passed after rejecting a drifted plain
param-zero fallback when authoritative joined prepared ownership still existed
for both the select-carried and `EdgeStoreSlot` variants, leaving
`test_after.log` at the repo root.
