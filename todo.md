# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening `render_local_slot_guard_chain_if_supported()` so the compare-join
continuation path no longer falls back to the raw trailing rhs compare when an
authoritative prepared rhs branch condition exists but has drifted out of a
usable prepared contract, and by adding regressions for both the select-carried
and `EdgeStoreSlot` short-circuit variants.

## Suggested Next

Stay in Step 3.4 and inspect the remaining residual helper order after the
sealed rhs compare-join fallback, especially any other
`render_local_slot_guard_chain_if_supported()` continuations or plain-condition
lanes that still permit raw compare carriers to rescue a route after prepared
branch or join metadata has become authoritative.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not widen into
  Step 3.2 compare-join reproving, Step 3.3 carrier hunting, Step 4 file
  organization, idea 57, idea 59, idea 60, idea 61, or the unrelated
  `^backend_` semantic-lowering failures.
- The local-slot short-circuit rhs continuation must now reject once a prepared
  rhs branch condition exists but no longer yields a valid prepared compare
  context; do not reopen the raw trailing-compare fallback behind that
  authoritative contract.
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
The focused Step 3.4 handoff proof passed after rejecting drifted prepared rhs
branch metadata in both select-carried and `EdgeStoreSlot` local short-circuit
routes instead of falling through to the raw rhs compare, leaving
`test_after.log` at the repo root.
