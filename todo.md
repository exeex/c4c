# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1.3.2
Current Step Title: Move Remaining Prepared I32 Binary And Select Render Helpers Behind Lowering Owners
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 2.1.3.2 by moving the remaining prepared-side `I32`
binary/select register-move and result-publication helpers behind lowering
owners:
`memory_lowering.*` now owns the `eax`/`ecx` carry-forward policy for prepared
`I32` binary rendering, the shared prepared `I32` select body builder, and the
named-`I32` result-publication helper used by these paths, while
`prepared_local_slot_render.cpp` now stays wrapper-thin around source lookup,
labels, and lowering-owned publication.

## Suggested Next

Advance step 2.1.4 by auditing the remaining frame/memory compatibility
holdouts under `src/backend/mir/x86/codegen/` so later lowering-family packets
inherit explicit ownership boundaries instead of mixed helper drift.

## Watchouts

- Keep the prepared-side overload wrapper thin: it should only bridge
  authoritative per-instruction memory rendering into lowering and reject any
  path that still needs setup asm.
- The new block-aware lowering helper only forwards direct frame-slot `DWORD`
  memory operands; if a future packet needs pointer-setup or symbol-backed
  `I32` operand materialization, handle that as a separate ownership move
  instead of silently broadening this seam.
- `render_prepared_named_stack_memory_operand_if_supported(...)` is a frame/home
  stack fallback helper only; do not widen it into symbol memory, ABI policy,
  or prepared entry-surface rewiring.
- Reject testcase-shaped matcher growth while moving the remaining `I32`
  operand/render helpers: the route is about ownership migration, not named-case
  shortcuts.

## Proof

Step 2.1.3.2 prepared-home `I32` binary/select lowering migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
