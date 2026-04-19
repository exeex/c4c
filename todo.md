# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3.4 Loop-Carry And Residual Consumer Cleanup packet by
shrinking `render_loop_join_countdown_if_supported()` from an unbounded
entry-to-preheader transparent-chain walk to a bounded entry-prefix legality
check that still keeps the prepared preheader handoff authoritative for the
two focused routes already under proof: direct `entry -> preheader -> loop`
and `entry -> carrier -> preheader -> loop` with one empty transparent
carrier.

## Suggested Next

Stay in Step 3.4 and target the next residual legality seam in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, starting with whether
`block_has_supported_init_handoff_carrier()` can be narrowed further around
prepared preheader handoff acceptance without reintroducing CFG-shape recovery
or widening into Step 4 emitter reorganization.

## Watchouts

- Keep this route in Step 3.4 residual consumer cleanup; do not slide back
  into Step 3.2 compare-join reproving, generic Step 3.3 carrier hunting,
  Step 4 file organization, idea 57, idea 59, idea 60, idea 61, or the
  unrelated `^backend_` semantic-lowering failures.
- This packet intentionally narrowed the accepted entry-to-preheader prefix to
  the two focused prepared routes already covered by the handoff tests; if a
  longer transparent carrier chain becomes necessary later, treat that as an
  explicit Step 3.4 route decision instead of quietly regrowing a generic CFG
  walk.
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
The focused Step 3.4 handoff proof passed after narrowing the residual
entry-prefix legality check, preserving the trivial-preheader and single
transparent-carrier countdown routes and leaving `test_after.log` at the repo
root.
