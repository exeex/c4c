Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Grouped Width-Greater-Than-One Allocation Truthful
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Step 3 now makes the x86 module emitter consume grouped authority through the
shared `consume_plans` seam before it falls back to the contract-first stub
body. Grouped functions now emit comment summaries and detail lines for shared
saved/preserved/clobber/storage authority plus grouped spill/reload value-id,
span, and spill-slot facts, and the grouped spill/reload contract fixture
proves that emitted asm reads those prepared facts directly.

## Suggested Next

Use the widened x86 module-emitter surface to find the next x86-native grouped
call-boundary consumer seam, or shift from consumer comments back to the next
truthful Step 3 behavior gap if the remaining width-`> 1` issue is now in
allocator behavior rather than consumer publication.

## Watchouts

- Keep grouped-width proofs anchored to shared prepared plans; do not let x86
  rebuild regalloc or spill facts from target-local heuristics.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; grouped-width progress must generalize
  beyond one named spill/reload case.
- Prefer cross-plan identity checks such as value id, register span, and spill
  slot authority over brittle target spelling that may move for unrelated
  reasons.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed on this packet.
Log: `test_after.log`
