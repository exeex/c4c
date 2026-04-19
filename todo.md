# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3.4
Current Step Title: Loop-Carry And Residual Consumer Cleanup
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening `render_local_i16_arithmetic_guard_if_supported()` so the residual
local-slot `i16` increment-guard lane now consumes the authoritative prepared
entry branch contract instead of raw entry labels, and by adding a regression
that drifts the prepared `false_label` after prepare to prove the emitter now
rejects that route instead of silently falling back to the raw guard matcher.

## Suggested Next

Stay in Step 3.4 and inspect the remaining generic fallback helpers after the
authoritative compare/join consumers, especially
`render_local_slot_guard_chain_if_supported()` and the residual local countdown
lane, for any other route where prepared branch/join or loop ownership can
still be bypassed after post-prepare module drift.

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
- The generic local `i32` guard fallback must now also reject once
  authoritative branch-owned short-circuit join ownership remains active for
  the same entry block; do not let later local guard cleanups reopen that
  bypass behind a valid-looking prepared branch record.
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
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3.4 handoff proof passed after the residual local-slot `i16`
increment guard began honoring the prepared branch contract and rejected a
drifted prepared `false_label`, leaving `test_after.log` at the repo root.
