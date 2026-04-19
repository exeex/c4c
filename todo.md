# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
blocking `render_local_i32_countdown_loop_if_supported()` from accepting a
countdown route once authoritative prepared loop-carry metadata exists, and by
adding a regression that drifts only the prepared branch-condition record to
prove x86 now rejects that route instead of falling back to local countdown
topology matching.

## Suggested Next

Stay in Step 3.4 and inspect the remaining residual consumer helpers around
the local guard/countdown fallbacks for any other cases where prepared branch
or join metadata can drift out of contract but an older local matcher still
accepts the function shape.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not slide back
  into Step 3.2 compare-join reproving, generic Step 3.3 carrier hunting,
  Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- The local countdown fallback must stay unavailable once a function carries
  authoritative `PreparedJoinTransferKind::LoopCarry` ownership; do not reopen
  that bypass by letting local slot/topology matching outrank prepared loop
  metadata drift.
- Keep proving that x86 consumes prepared loop-carry ownership and init values
  through prepared metadata even when legalized entry/preheader/body stores or
  carrier topology drift after prepare, rather than re-reading those values
  from local carrier code.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3.4 handoff proof passed after rejecting loop-countdown routes
whose prepared branch metadata drifts out of contract instead of falling
through to the local countdown fallback, leaving `test_after.log` at the repo
root.
