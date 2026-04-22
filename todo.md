# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1.3
Current Step Title: Move Prepared-Home Selection And Memory Render Helpers Behind Lowering Owners
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Continued step 2.1.3 by moving the next prepared-home `I32` local-slot and
block-source fallback helper family behind lowering owners:
`memory_lowering.*` now owns
`select_prepared_named_i32_block_source_if_supported(...)`, and
`frame_lowering.*` now owns
`render_prepared_named_stack_memory_operand_if_supported(...)` for
local-slot/value-home stack operand fallback, so
`prepared_local_slot_render.cpp` now stays as a thin bridge that only
supplies setup-free prior-load memory operands instead of composing
lowered home resolution inline.

## Suggested Next

Continue step 2.1.3 by moving the next `I32` operand adapter behind lowering
owners, starting with `render_prepared_named_i32_operand_if_supported(...)`
so the remaining compare/call bundle paths stop translating
lowering-owned `PreparedNamedI32Source` values into operand strings inside
`prepared_local_slot_render.cpp`.

## Watchouts

- Keep the prepared-side overload wrapper thin: it should only bridge
  authoritative per-instruction memory rendering into lowering and reject any
  path that still needs setup asm.
- `render_prepared_named_stack_memory_operand_if_supported(...)` is a frame/home
  stack fallback helper only; do not widen it into symbol memory, ABI policy,
  or prepared entry-surface rewiring.
- Reject testcase-shaped matcher growth while moving the remaining `I32`
  operand/render helpers: the route is about ownership migration, not named-case
  shortcuts.

## Proof

Step 2.1.3 prepared-home `I32` block-source lowering migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
