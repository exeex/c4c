# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Residual Prepared-Module Dispatch Narrowing
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed Step 4.3 by moving
`render_local_i16_i64_sub_return_if_supported()` out of
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and onto the existing
prepared local-slot helper seam in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` plus
`src/backend/mir/x86/codegen/x86_codegen.hpp`, so the emitter now keeps only
dispatch for that compact local-slot return lane instead of its direct route
body.

## Suggested Next

Keep Step 4.3 bounded to the remaining top-level single-block return dispatch
surface in `prepared_module_emit.cpp`, likely by extracting one coherent
immediate/passthrough/param-derived return helper only if it honestly removes
another residual route body without reopening Step 3 semantics or turning
Step 4 into indefinite cosmetic churn.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- The active Step 4 seams now include the prepared param-zero / branch helper
  surface, the prepared countdown helper surface, the prepared local-slot
  guard/arithmetic/compact-return helper surface, and the residual
  trivial-function / direct-extern-call helper surface. Do not reopen Step 3
  short-circuit semantics or hide value-home/addressing work inside those
  APIs.
- The remaining `prepared_module_emit.cpp` return ladder still mixes compact
  immediate and param-derived route bodies with dispatcher glue. Treat any
  follow-on Step 4.3 packet as valid only if it removes one of those route
  bodies cleanly through an existing seam.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 4.3 packet after moving the compact local `i16`/`i64`
sub-return helper body onto the existing prepared local-slot helper seam; the
proof passed and `test_after.log` is the canonical proof log.
