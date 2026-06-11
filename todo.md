Status: Active
Source Idea Path: ideas/open/176_return_chain_owner_schema_decision.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Decide Owner And Schema Boundary

# Current Packet

## Just Finished

Completed Step 2 - Decide Owner And Schema Boundary. Chose BIR-owned
return-chain identity and recorded the proposed record/key shape, accepted and
rejected cases, stability rules, target-policy boundaries, and rejected
target-local/prepared-owned alternatives in
`docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`.

## Suggested Next

Proceed to Step 3 by defining the oracle proof route and follow-up
implementation ideas for the BIR-owned return-chain route without changing
implementation files inside this analysis plan.

## Watchouts

- The chosen BIR owner is only for target-neutral BIR value-chain identity:
  function/block, instruction position or reference, chain value, terminal
  return value, and optional immediate next-operand value.
- AArch64 still owns homes, ABI return moves, register parsing/conversion,
  alias checks, scratch choice, scalar ALU record construction, and emission
  order.
- Follow-up work must keep prepared helpers public as migration oracles until
  positive, negative, ambiguous, and conflict cases prove BIR/prepared
  equivalence.

## Proof

No build run; delegated proof was metadata/analysis-only and no implementation
or test files were changed.
