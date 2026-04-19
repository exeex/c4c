# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 3.4
Current Step Title: Loop-Carry And Residual Consumer Cleanup
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening `render_local_i32_arithmetic_guard_if_supported()` so the generic
local-slot compare-against-immediate guard lane now rejects fallback once
authoritative branch-owned short-circuit join ownership is still published for
the entry block, and by adding a regression that mutates the prepared
short-circuit fixture into a three-block local guard shape after prepare to
prove the emitter now throws instead of bypassing the canonical prepared
handoff.

## Suggested Next

Stay in Step 3.4 and inspect the remaining generic fallback helpers after the
authoritative compare/join consumers, especially the local-slot guard-chain
and adjacent residual guard/countdown lanes, for any other route where active
prepared branch-owned join or loop ownership can still be bypassed after
post-prepare module drift.

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
The focused Step 3.4 handoff proof passed after rejecting a drifted generic
local-slot `i32` guard fallback when authoritative short-circuit join
ownership still existed for the entry block, leaving `test_after.log` at the
repo root.
