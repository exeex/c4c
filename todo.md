# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Stand Up Frame And Memory Lowering Owners
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Continued step 2.1 by moving the next canonical frame-home query family out of
`prepared_local_slot_render.cpp`: authoritative stack-offset lookup now routes
through `lowering/frame_lowering.*`, stack-slot home fallback/base-offset
queries live there too, and value-home stack-address rendering now routes
through `lowering/memory_lowering.*` instead of the mixed prepared
compatibility file.

## Suggested Next

Continue step 2.1 by moving the next remaining frame-home consumers behind the
same seams, preferably the prepared pointer/byval payload helpers that still
reconstruct slot-or-home offsets inline instead of calling the lowering-owned
query helpers directly.

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

## Proof

Step 2.1 frame-home query migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Canonical log paths: `test_before.log`, `test_after.log`
