# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.1.4
Current Step Title: Audit Remaining Frame/Memory Holdouts Before Later Lowering Families
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 2.1.4 by moving the remaining pointer-value scalar-memory
operand/setup helpers behind lowering-owned seams: `memory_lowering.*` now owns
prepared named-pointer home lookup, pointer scratch selection, pointer-family
carrier tracking, authoritative pointer-memory-use detection, generic
pointer-value memory-operand rendering, and the entry pointer-param access-base
/ scalar-memory operand selection used by the same-module helper prefix. The
prepared renderer now keeps only the typed load/result-state transitions and
the final stack-home refresh store as the explicit compatibility boundary.

## Suggested Next

Advance out of 2.1.4 into the next planned lowering family, or do a quick
review pass on whether the remaining prepared-only pointer-param home-refresh
store should stay as the compatibility wrapper until a later ABI/home-lowering
step owns that writeback surface.

## Watchouts

- Keep the remaining prepared-side pointer-param wrapper thin: it should only
  perform typed load/result-state bookkeeping plus the explicit stack-home
  writeback, with access-base lookup and memory-operand selection staying in
  `memory_lowering.*`.
- Same-module global tracking in the handoff helpers still depends on the
  lowering-owned symbol-access query; preserve current bookkeeping if later
  cleanup touches that surface.
- The surviving pointer-param home-refresh store is ABI/home-policy flavored;
  do not fold broader ABI rewiring into generic memory-lowering helpers unless
  a later step explicitly takes ownership of that family.
- Reject testcase-shaped matcher growth while continuing the ownership cleanup:
  this route is about seam control, not named-case shortcuts.

## Proof

Step 2.1.4 frame/symbol scalar-memory ownership audit on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
