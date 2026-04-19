# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
tightening the remaining transparent entry-carrier prefix acceptance in
`render_loop_join_countdown_if_supported()` around a unique linear prepared
init-handoff path, so loop countdown support no longer relies on a broad
single-empty-carrier allowance and now rejects a non-authoritative transparent
prefix even when the carrier block stays empty and branch-only.

## Suggested Next

Stay in Step 3.4 and inspect the remaining loop-countdown and residual-consumer
helpers for any other raw CFG-prefix checks that should consume the same
prepared init-handoff authority instead of local carrier shape.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not slide back
  into Step 3.2 compare-join reproving, generic Step 3.3 carrier hunting,
  Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- The countdown helper now accepts transparent entry prefixes only when the
  path from `entry` to the prepared init predecessor stays unique, linear, and
  instruction-free ahead of the authoritative handoff carrier; do not regrow
  one-off empty-block allowances that skip predecessor uniqueness.
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
The focused Step 3.4 handoff proof passed after replacing the raw transparent
entry-carrier allowance with a unique-prefix validator and adding a regression
that rejects a non-authoritative transparent entry prefix, leaving
`test_after.log` at the repo root.
