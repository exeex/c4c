Status: Active
Source Idea Path: ideas/open/866_lir_remaining_authority_owner_triage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Build the current evidence bundle

# Current Packet

## Just Finished

Lifecycle switched from concluded no-change 865 to successor umbrella 866.
Idea 734 remains paused after accepted receiver commit `750b6b3ba`; 865
produced no handoff because its selected `LirAbsOp` selected-global/i32 row is
already received by `0c44e810ad`.

## Suggested Next

Execute Step 1 by creating `docs/lir_remaining_authority_owner_triage/` and a
current-evidence document that records the accepted commits, rejected 865 row,
accepted rows not to reopen, and broad remaining families to classify.

## Watchouts

Do not implement code or tests in this umbrella. Do not reopen the accepted
`LirAbsOp` selected-global/i32 receipt, direct-call argument 0/1 receipts,
body-parameter receipts, local-object/VLA receipts, or accepted call-result
rows. Stale open ideas need reconciliation before they can become current 734
successors.

## Proof

Lifecycle-only switch pending validation. Minimum proof: `git diff --check`.
