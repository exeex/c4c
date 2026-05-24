Status: Active
Source Idea Path: ideas/open/prealloc-regalloc-coordinator-contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Coordinator And Helper Families

# Current Packet

## Just Finished

Activated Step 1 from `plan.md`; no execution packet has run yet.

## Suggested Next

Run Step 1 inventory for `src/backend/prealloc/regalloc.cpp` and the existing
`src/backend/prealloc/regalloc/` helper family. Record the first safe Step 2
candidate in this file.

## Watchouts

Do not change allocation behavior, spill/reload behavior, interval
construction, liveness semantics, ABI facts, or prepared-printer dump meaning
under an inventory packet.

## Proof

Activation-only lifecycle work; run `git diff --check` before supervisor
commit.
