Status: Active
Source Idea Path: ideas/open/353_aarch64_local_formal_frame_slot_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Formal To Local Slot Publication Boundary

# Current Packet

## Just Finished

Lifecycle activated idea 353 after idea 352 became source-complete but could
not be closed under the current monotonic regression-log gate.

## Suggested Next

Execute plan Step 1: localize the AArch64 scalar formal-to-local frame-slot
publication boundary using `00176` `partition` as the representative and focused
backend local/load-store evidence as the owned proof path.

## Watchouts

- Keep idea 352's repaired `.LBB90_6`/`.LBB90_7` branch/label path intact.
- Do not special-case `00176`, `partition`, one local name, one stack offset,
  one formal register, or one emitted instruction sequence.
- Do not broaden into variadic, byval, HFA, aggregate writeback, or broad frame
  layout without fresh first-bad-fact evidence.

## Proof

No execution proof has run for idea 353 yet. Closure of idea 352 was rejected
by the close-time guard because `test_before.log` and `test_after.log` both
showed 5/6 in the focused subset, so the pass count did not strictly increase.
