# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1.3
Current Step Title: Move Prepared-Home Selection And Memory Render Helpers Behind Lowering Owners
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Continued step 2.1.3 by moving the remaining block-aware prepared-side `I32`
operand-selection/render forwarding seams behind lowering owners:
`memory_lowering.*` now owns the block-aware named/value source selectors and
named operand renderer for prepared `I32` inputs, and
`prepared_local_slot_render.cpp` no longer builds local adapter lambdas around
those touched block-aware selection/render paths.

## Suggested Next

Continue step 2.1.3 by migrating the remaining prepared-side `I32` helpers
that still duplicate small home-sync or register-move decisions around
lowering-owned `PreparedNamedI32Source` flows, without widening into frame
lowering or entry-surface rewiring.

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

Step 2.1.3 prepared-home `I32` operand adapter lowering migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
