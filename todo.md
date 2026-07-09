Status: Active
Source Idea Path: ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Callee-Saved Placement Evidence

# Current Packet

## Just Finished

Activated idea 626 into plan.md. No execution packet has run yet.

## Suggested Next

Run Step 1 diagnostics for src/20040811-1.c, src/pr43220.c, and
src/vla-dealloc-1.c to refresh prepared frame facts and RV64 object-route
failure buckets before implementation.

## Watchouts

- Do not infer callee-saved save-slot placements in RV64 from frame size,
  register order, source filename, or final assembly shape.
- Keep FPR placement, move-bundle authority, local/global memory repair, call
  policy, and expectation changes outside this idea.
- Preserve dynamic stack operation metadata and existing frame size/alignment
  facts while adding placement authority.

## Proof

Lifecycle-only activation. No build or test proof required.
