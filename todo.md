Status: Active
Source Idea Path: ideas/open/860_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and publish one next body-parameter authority row
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Switched away from 734 after accepted Step 7.48 receiver commit `6a91d07ca`.
The new active packet is the producer-side blocker that must publish exactly
one next structured body-parameter authority row for a later 734 receiver.

## Suggested Next

Inspect accepted body-parameter authority patterns through 734 Step 7.48,
select one next valid current-LIR row, and implement only the LIR
producer/schema/verifier/test handoff for that row.

## Watchouts

Do not edit Raw-BIR receiver code, reopen accepted DirectPointer or
DirectScalar receipts, select multiple rows, recover authority from
presentation text, or absorb memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
ABI-expanded or aggregate parameters, or target-lowering work.

## Proof

No proof has run for Step 1 yet.
