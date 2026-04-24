Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Grouped Width-Greater-Than-One Allocation Truthful
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3 now publishes prepared regalloc through the x86 `consume_plans` seam so
downstream helpers can read grouped spill/reload authority directly from the
shared prepared packet. The grouped spill/reload contract and prepared-printer
fixtures now prove that width-`> 1` spill/reload facts stay truthful on that
consumer seam, including the grouped register span, spill slot identity, and
value-id linkage without x86-local reconstruction.

## Suggested Next

Use the new x86-published regalloc seam to tighten the next grouped-width
packet around another downstream consumer that still reconstructs span or spill
facts locally, or move to the next Step 3 proof gap if grouped width `> 1`
authority still disappears outside the current spill/reload path.

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
