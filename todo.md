Status: Active
Source Idea Path: ideas/open/177_bir_return_chain_schema_index.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Route and Return-Chain Shapes

# Current Packet

## Just Finished

Lifecycle activation only. No implementation packet has run yet.

## Suggested Next

Start Step 1 by inspecting the existing BIR route/index patterns, the prepared
return-chain helper shape, and the focused backend/BIR test style. Record the
chosen schema/index/test targets here before making code changes.

## Watchouts

- Keep the route target-neutral: no homes, registers, ABI placement, scratch
  policy, alias checks, ALU record construction, or emission order.
- Do not migrate AArch64 consumers or hide prepared return-chain helpers in
  this idea.
- Do not choose a winner for conflicting duplicate return-chain answers.
- Keep oracle equivalence against prepared helpers for idea 178.

## Proof

Not run; lifecycle-only activation.
