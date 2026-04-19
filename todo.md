# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening `block_has_supported_init_handoff_carrier()` so the non-entry init
predecessor must still own the prepared loop-carry handoff via the matching
`StoreLocalInst`, instead of accepting an arbitrary empty block as a valid
preheader handoff carrier.

## Suggested Next

Stay in Step 3.4 and inspect whether the remaining `init_block == &entry`
fast-path in `render_loop_join_countdown_if_supported()` can be narrowed
further around authoritative prepared init-value ownership without widening
into Step 4 emitter reorganization or regrowing CFG-shape recovery.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not slide back
  into Step 3.2 compare-join reproving, generic Step 3.3 carrier hunting,
  Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- This packet intentionally removed the residual allowance for non-entry empty
  init carriers. The covered preheader routes still pass because prepare keeps
  the authoritative handoff on the matching preheader store; do not regrow
  generic empty-block acceptance without an explicit Step 3.4 route decision.
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
The focused Step 3.4 handoff proof passed after narrowing residual init-carrier
acceptance to the matching prepared preheader-store lane, leaving
`test_after.log` at the repo root.
