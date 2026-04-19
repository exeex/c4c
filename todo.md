# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3.4
Current Step Title: Loop-Carry And Residual Consumer Cleanup
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
hardening the residual two-segment local countdown fallback so it also rejects
authoritative prepared guard-branch ownership on the mixed
guard/continuation split instead of reopening raw countdown topology, and by
adding a focused handoff regression for that contract.

## Suggested Next

Stay in Step 3.4 and inspect the remaining residual local countdown fallback
for any adjacent post-prepare drift that can still bypass the canonical
prepared-module handoff through unmatched countdown sub-blocks beyond the
guard-owned continuation split now covered here.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not widen into
  Step 3.2 compare-join reproving, Step 3.3 carrier hunting, Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- The plain equality-against-immediate guard-chain lane must now reject once
  an authoritative prepared branch contract exists for that block but no
  longer resolves valid prepared branch targets; do not let later helper
  cleanup turn that failure back into a soft route miss.
- The local-slot short-circuit rhs continuation must now reject once a prepared
  rhs branch condition exists but no longer yields a valid prepared compare
  context; do not reopen the raw trailing-compare fallback behind that
  authoritative contract.
- The local-slot short-circuit entry branch must now reject once authoritative
  branch-owned join ownership exists but the entry prepared branch contract has
  drifted; do not let that route fall through into the generic plain-condition
  lane.
- The generic local `i32` guard fallback must now also reject once
  authoritative branch-owned short-circuit join ownership remains active for
  the same entry block; do not let later local guard cleanups reopen that
  bypass behind a valid-looking prepared branch record.
- The same-module-global and pointer-backed guard-chain variants now have the
  same drifted-label regression shape as the non-global guard-chain lane; keep
  those routes on the shared authoritative prepared-branch contract instead of
  growing variant-specific fallback behavior.
- The residual local `i16` increment guard must now treat the prepared entry
  branch labels and compare contract as authoritative; do not let later helper
  cleanups reopen the raw entry-label fallback when the prepared branch record
  drifts after prepare.
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
- The local countdown fallback must now also stay unavailable when the matched
  countdown block still has authoritative prepared branch ownership even if the
  loop transfer drifts away from `PreparedJoinTransferKind::LoopCarry`; do not
  let later helper cleanup rely on the raw countdown matcher in that state.
- The optional guard block inside the residual two-segment local countdown
  fallback must now also reject once authoritative prepared branch ownership
  exists for that guard/continuation split; do not let later cleanup reopen
  the raw guard matcher behind that contract.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3.4 handoff proof passed with the new residual loop-countdown
transfer-drift regression, leaving `test_after.log` as the canonical proof
log.
