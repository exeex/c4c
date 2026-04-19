# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by making
`render_loop_join_countdown_if_supported()` take the init handoff value from
authoritative prepared loop-carry metadata instead of re-validating the
legalized preheader store literal, while still keeping the transparent
preheader and entry-carrier chain legality checks bounded to structural
carriers. Added focused loop-countdown handoff regressions in
`tests/backend/backend_x86_handoff_boundary_test.cpp` that mutate the
preheader store, loop compare, and body store after prepare and prove x86
still follows the prepared loop contract for both the trivial-preheader and
transparent-carrier-chain routes.

## Suggested Next

Stay in Step 3.4 and target the next concrete residual loop-carried consumer
seam in `src/backend/mir/x86/codegen/prepared_module_emit.cpp`, starting with
whether the remaining entry-to-preheader transparent-chain walk can be reduced
further without widening beyond legality checks or sliding into Step 4 file
organization, then prove that slice with the same focused handoff-boundary
coverage if the changed route stays inside this test family.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not slide back
  into Step 3.2 compare-join reproving, generic Step 3.3 carrier hunting,
  Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- This packet intentionally removed raw-BIR init-value validation from the
  preheader carrier path, but it did not declare the transparent carrier walk
  itself exhausted; treat any follow-on work there as legality cleanup only,
  not as a license to reintroduce ownership recovery from CFG shape.
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
The focused Step 3.4 handoff proof passed with the new preheader-handoff
regressions for both the trivial-preheader and transparent entry-carrier chain
loop-countdown routes, and preserved `test_after.log` at the repo root.
