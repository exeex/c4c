Status: Active
Source Idea Path: ideas/open/866_lir_remaining_authority_owner_triage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify remaining families by first owner

# Current Packet

## Just Finished

Completed Step 1 by creating
`docs/lir_remaining_authority_owner_triage/current_evidence.md`. The evidence
records that 734 is accepted through Step 7.51 at `750b6b3ba`, that 865
produced no handoff, that `LirAbsOp` selected-global/i32 is already received
by `0c44e810ad`, and that accepted/stale rows must not be reopened.

## Suggested Next

Execute Step 2 by classifying CFG/PHI residuals, memory/VA,
aggregate/vector, module/type/global/metadata, residual
instruction/terminator, inline-assembly, generic residual sweeps, and stale
overlapping open ideas by first owning layer.

## Watchouts

Do not implement code or tests in this umbrella. Do not reopen the accepted
`LirAbsOp` selected-global/i32 receipt, direct-call argument 0/1 receipts,
body-parameter receipts, local-object/VLA receipts, or accepted call-result
rows. Stale open ideas need reconciliation before they can become current 734
successors.

## Proof

Docs-only Step 1 proof: `git diff --check`. No `test_after.log` was written
because the delegated proof command is documentation-only and produces no
test log.
