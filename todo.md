# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Stand Up Frame And Memory Lowering Owners
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Continued step 2.1 by moving the aggregate slice/root-home prepared stack-home
helper in `prepared_local_slot_render.cpp` behind the reviewed lowering
owners: `render_prepared_aggregate_slice_root_home_memory_operand_if_supported(...)`
now resolves exact root-home and ancestor/root fallback addresses through
`find_prepared_value_home_frame_offset(...)` and
`render_prepared_named_stack_address_if_supported(...)` instead of rebuilding
prepared home lookup and authoritative-offset fallback inline.

## Suggested Next

Continue step 2.1 by moving the next remaining prepared stack/home consumers in
`prepared_local_slot_render.cpp` behind the same seams, especially helper
paths that still call `find_prepared_value_home(...)` or
`find_prepared_value_home_frame_offset(...)` directly for operand/home rebuilds
instead of delegating through lowering-owned frame/memory queries.

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
- This packet intentionally kept the aggregate-root ancestor walk local, but
  each lookup now delegates through lowering-owned frame/memory queries rather
  than open-coding prepared-home extraction; keep the next slice focused on the
  remaining inline prepared-home consumers, not on unrelated register routing.

## Proof

Step 2.1 aggregate slice/root-home lowering migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Canonical log paths: `test_before.log`, `test_after.log`
