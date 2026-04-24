Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make Grouped Width-Greater-Than-One Allocation Truthful
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Step 3 now proves the x86 module emitter reads grouped authority through the
shared `consume_plans` seam for both grouped call-boundary and grouped
spill/reload fixtures before it falls back to the contract-first stub body.
Grouped functions now have direct backend contracts for emitted comment
summaries and detail lines covering shared saved/preserved/clobber/storage
authority plus grouped spill/reload value-id, span, and spill-slot facts.

## Suggested Next

Use the widened grouped proofs to identify the next truthful Step 3 gap beyond
consumer publication, or move to the next x86-native grouped-width behavior
that still lacks direct prepared-authority proof.

## Watchouts

- Keep grouped-width proofs anchored to shared prepared plans; do not let x86
  rebuild regalloc or spill facts from target-local heuristics.
- Keep grouped call-boundary proofs tied to preserved-value, saved-register,
  caller-clobber, and storage identities from prepared authority rather than
  brittle asm spelling outside the consumer seam.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; grouped-width progress must generalize
  beyond one named spill/reload case.
- Prefer cross-plan identity checks such as value id, register span, and spill
  slot authority over brittle target spelling that may move for unrelated
  reasons.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed after adding the grouped call-boundary x86 module-emitter
contract alongside the grouped spill/reload contract.
Log: `test_after.log`
