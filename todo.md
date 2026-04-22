# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1.4
Current Step Title: Audit Remaining Frame/Memory Holdouts Before Later Lowering Families
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed step 2.1.4 by moving the duplicated prepared frame-slot/global-symbol
scalar-memory operand helpers behind lowering-owned seams:
`memory_lowering.*` now owns scalar size-name selection, frame-slot/global-symbol
prepared-memory access lookup, and the compatible per-instruction scalar-memory
operand render path, while `prepared_local_slot_render.cpp` keeps only the
pointer-value fallback as an explicit compatibility boundary for setup-asm cases.

## Suggested Next

Take the next 2.1.4 packet on the remaining pointer-value scalar-memory holdout:
either move the pointer setup/render path behind a lowering-owned seam or record
why it must stay prepared-only before later float/call family packets build on
this boundary.

## Watchouts

- Keep the prepared-side wrapper thin: it should accept lowering-owned
  frame/symbol compatibility results first, then branch explicitly into the
  pointer-value setup path instead of regrowing mixed ownership.
- Same-module global tracking in the handoff helpers still depends on the
  symbol-access query; if that path moves again, preserve the current tracking
  behavior rather than silently dropping global-usage bookkeeping.
- `render_prepared_named_stack_memory_operand_if_supported(...)` remains a
  frame/home stack fallback helper only; do not widen it into symbol memory,
  ABI policy, or prepared entry-surface rewiring.
- Reject testcase-shaped matcher growth while auditing the remaining holdouts:
  this route is about ownership boundaries, not named-case shortcuts.

## Proof

Step 2.1.4 frame/symbol scalar-memory ownership audit on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
