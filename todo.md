# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Residual Prepared-Module Dispatch Narrowing
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Completed Step 4.3 by moving the residual single-block
immediate/passthrough/param-immediate return lane out of
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and onto the existing
prepared local-slot return seam via
`render_prepared_param_derived_i32_value_if_supported()` and
`render_prepared_minimal_immediate_or_param_return_if_supported()` in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` plus
`src/backend/mir/x86/codegen/x86_codegen.hpp`, so the emitter now delegates
that compact direct-return body instead of owning it inline.

## Suggested Next

Keep Step 4.3 bounded to any remaining top-level
`prepared_module_emit.cpp` dispatch body only if another existing helper seam
can honestly absorb it, likely around the residual trivial-function or
direct-extern single-block surface rather than reopening prepared control-flow
consumption work.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- The active Step 4 seams now include the prepared param-zero / branch helper
  surface, the prepared countdown helper surface, the prepared local-slot
  guard/arithmetic/compact-return helper surface including direct
  immediate/param returns, and the residual
  trivial-function / direct-extern-call helper surface. Do not reopen Step 3
  short-circuit semantics or hide value-home/addressing work inside those
  APIs.
- The remaining `prepared_module_emit.cpp` work is increasingly narrow
  dispatcher glue. Treat any follow-on Step 4.3 packet as valid only if it
  removes another concrete route body through an existing seam instead of
  shuffling orchestration lines around.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 4.3 packet after moving the residual single-block
immediate/passthrough/param-immediate return lane onto the existing prepared
helper seam; the proof passed and `test_after.log` is the canonical proof
log.
