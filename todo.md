# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Stand Up Frame And Memory Lowering Owners
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Continued step 2.1 by moving the named i32/ptr prepared home-sync family in
`prepared_local_slot_render.cpp` behind the reviewed lowering owners:
`render_prepared_named_i32_home_sync_if_supported(...)`,
`render_prepared_named_i32_stack_home_sync_if_supported(...)`, and
`render_prepared_named_ptr_home_sync_if_supported(...)` now delegate to
lowering-owned `resolve_prepared_named_home_if_supported(...)` and
lowering-owned memory render helpers instead of calling
`find_prepared_value_home(...)` directly or rebuilding stack-home offsets from
prepared-home fields inline.

## Suggested Next

Continue step 2.1 by moving the remaining scalar load/store prepared-home
selection helpers in `prepared_local_slot_render.cpp` behind the same seams,
especially the float/scalar local-load paths near the late block renderer that
still inspect `PreparedValueHome` directly to choose register versus stack
destinations.

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
- This packet intentionally kept the broader scalar and float load/store
  destination selection local; the next slice should peel those consumers onto
  lowering-owned selection/render helpers instead of widening into compare,
  call, or extern routing.

## Proof

Step 2.1 named i32/ptr home-sync lowering migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Canonical log paths: `test_before.log`, `test_after.log`
