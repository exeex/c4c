# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Stand Up Frame And Memory Lowering Owners
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Continued step 2.1 by moving the prepared pointer/byval payload helper family
out of `prepared_local_slot_render.cpp`: published payload frame-offset
resolution now lives in `lowering/frame_lowering.*`, named stack-address
rendering for pointer/value-home cases routes through `lowering/memory_lowering.*`,
and the prepared helpers now call those owners instead of rebuilding slot/home
offsets inline.

## Suggested Next

Continue step 2.1 by moving the next remaining prepared stack-address
consumers behind the same seams, preferably the other call/pointer helpers in
`prepared_local_slot_render.cpp` that still spell out direct `home->offset_bytes`
stack address reconstruction instead of using lowering-owned render helpers.

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
  only; do not widen step 2.1 into ABI policy or prepared entry-surface edits.

## Proof

Step 2.1 frame-home query migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Canonical log paths: `test_before.log`, `test_after.log`
