# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1.3
Current Step Title: Move Prepared-Home Selection And Memory Render Helpers Behind Lowering Owners
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Continued rewritten step 2.1.3 by moving the remaining prepared local-load
destination selection behind reviewed lowering owners: `memory_lowering.*` now
owns `render_prepared_named_qword_load_from_memory_if_supported(...)` and
`render_prepared_named_float_load_from_memory_if_supported(...)`, and the
late local `I64`/`F32`/`F64` load paths in
`prepared_local_slot_render.cpp` plus the shared float local-load helper now
delegate there instead of inspecting `PreparedValueHome` inline to choose
register versus stack destinations.

## Suggested Next

Continue step 2.1.3 by moving the next scalar prepared-home selection helpers
that still construct register/stack/immediate source choices inline in
`prepared_local_slot_render.cpp`, especially the `I32` source-selection and
home-sync families that still call `find_prepared_value_home(...)` directly
outside the new lowering-owned local-load helpers.

## Watchouts

- Keep `module_emit.cpp` and `route_debug.cpp` classified as prepared-route
  compatibility holdouts until real reviewed `prepared/` owners exist; do not
  fake step-2 progress by inventing new prepared headers.
- Treat step 2 as canonical lowering migration, not public-entry or prepared
  adapter rewiring; keep `emit.cpp`, `prepared_module_emit.cpp`, and
  route-debug surfaces wrapper-thin.
- Start with frame and memory services because later call, comparison, and
  prepared packets depend on canonical frame-home and operand helpers.
- Preserve the legacy `x86::emit_module(...)`, `x86::assemble_module(...)`,
  and `x86::emit_prepared_module(...)` compatibility surfaces until step 4
  explicitly rewires live entry ownership.
- Reject testcase-shaped matcher growth while moving helpers: the packet should
  relocate semantic lowering owners, not add new named-case shortcuts.
- Keep these new lowering helpers about frame-home and stack-address ownership
  only; do not widen step 2.1.3 into ABI policy or prepared entry-surface
  edits.
- This packet moved local-load destination placement only; `CastInst` float
  destination choice and the broader scalar source-selection families still
  live in `prepared_local_slot_render.cpp` and should be peeled off in later
  step-2.1.3 packets without widening into compare, call, or extern routing.

## Proof

Step 2.1.3 local-load destination lowering migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Canonical log paths: `test_before.log`, `test_after.log`
