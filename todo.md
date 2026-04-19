# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening the remaining direct-entry fast path in
`render_loop_join_countdown_if_supported()` so entry-backed loop countdowns
must satisfy the same prepared handoff carrier-lane check as non-entry init
predecessors, and proving that a drifted entry `StoreLocalInst` slot now
rejects the route instead of slipping through a broad `init_block == &entry`
allowance.

## Suggested Next

Stay in Step 3.4 and inspect whether the remaining transparent entry-carrier
prefix acceptance in `render_loop_join_countdown_if_supported()` can be
described with one tighter prepared-handoff helper without widening into Step
4 emitter reorganization or regrowing CFG-shape recovery.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not slide back
  into Step 3.2 compare-join reproving, generic Step 3.3 carrier hunting,
  Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- This packet intentionally removed the residual broad entry allowance as well
  as the prior non-entry empty-carrier allowance. The covered routes still pass
  because prepare keeps the authoritative init handoff on the matching carrier
  lane; do not regrow generic entry or empty-block acceptance without an
  explicit Step 3.4 route decision.
- Keep proving that x86 consumes prepared loop-carry ownership and init values
  through prepared metadata even when legalized preheader/body stores drift
  after prepare, rather than re-reading those values from local carrier code.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused Step 3.4 handoff proof passed after narrowing direct-entry loop
countdown acceptance to the matching prepared carrier lane and adding a
regression that rejects a drifted entry handoff slot, leaving `test_after.log`
at the repo root.
