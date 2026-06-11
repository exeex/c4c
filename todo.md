Status: Active
Source Idea Path: ideas/open/178_bir_return_chain_oracle_equivalence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Oracle Surfaces and Fixture Keys

# Current Packet

## Just Finished

Lifecycle activation only. No implementation packet has run yet.

## Suggested Next

Start Step 1 by inspecting the Route 8 return-chain helpers, the prepared
return-chain helper key shape, and the existing backend/BIR lookup helper
fixtures. Record the shared key and chosen fixture/helper shape here before
making code changes.

## Watchouts

- Keep prepared return-chain helpers public and unchanged during this idea.
- Do not migrate AArch64 consumers or contract the prepared API.
- Compare Route 8 against prepared helper answers for terminal and
  next-operand queries.
- Rejected, ambiguous, and duplicate-conflict fixtures must fail closed rather
  than choosing a winner.

## Proof

Not run; lifecycle-only activation.
