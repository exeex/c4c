# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1.3
Current Step Title: Move Prepared-Home Selection And Memory Render Helpers Behind Lowering Owners
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Continued rewritten step 2.1.3 by moving the next prepared-home `I32`
source-selection and home-sync helpers behind reviewed lowering owners:
`memory_lowering.*` now owns `PreparedNamedI32Source`,
`select_prepared_named_i32_source_if_supported(...)`,
`select_prepared_i32_source_if_supported(...)`,
`render_prepared_i32_operand_from_source_if_supported(...)`, and
`append_prepared_named_i32_home_sync_if_supported(...)`, so
`prepared_local_slot_render.cpp` now delegates the reusable `I32`
register/stack/immediate source picking and destination home sync logic
through lowering-owned frame/memory helpers instead of inspecting
`PreparedValueHome` inline.

## Suggested Next

Continue step 2.1.3 by peeling the next `I32` prepared-home helper that still
mixes block-local slot fallbacks with lowered home resolution in
`prepared_local_slot_render.cpp`, especially
`select_prepared_named_i32_block_source_if_supported(...)` and adjacent
call-result/cast paths that still compose frame offsets and source-family
selection inline around the new lowering-owned `PreparedNamedI32Source`
helpers.

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
- The reusable `I32` home-driven helpers now live in `memory_lowering.*`, but
  `select_prepared_named_i32_block_source_if_supported(...)` still owns the
  local-slot and recent-block scan overlays in `prepared_local_slot_render.cpp`;
  keep that fallback layer separate from broader compare/call/extern rewrites.

## Proof

Step 2.1.3 prepared-home `I32` helper lowering migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed. Canonical log paths: `test_before.log`, `test_after.log`
