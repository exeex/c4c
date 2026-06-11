Status: Active
Source Idea Path: ideas/open/176_return_chain_owner_schema_decision.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Define Proof And Follow-Up Work

# Current Packet

## Just Finished

Completed Step 3 - Define Proof And Follow-Up Work. Added oracle proof
requirements to
`docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md` for terminal
and next-operand positive, negative, ambiguous, and conflict cases. Created
bounded BIR-owned follow-up ideas:
`ideas/open/177_bir_return_chain_schema_index.md`,
`ideas/open/178_bir_return_chain_oracle_equivalence.md`,
`ideas/open/179_bir_return_chain_consumer_migration.md`, and
`ideas/open/180_bir_return_chain_prepared_api_contraction.md`.

## Suggested Next

Proceed to Step 4 by closing the analysis loop: verify the runbook, decision
artifact, and new follow-up ideas agree on BIR-owned return-chain identity and
confirm no implementation, test, source-idea, closed-idea, review, or root-log
files were changed by this analysis packet.

## Watchouts

- The follow-up ideas are intentionally sequenced: schema/index first, oracle
  equivalence second, consumer migration third, prepared API contraction last.
- Prepared return-chain helpers must remain public until the oracle
  equivalence idea proves BIR/prepared parity for terminal-only,
  terminal-plus-next-operand, negative, ambiguous, and conflict cases.
- Consumer migration must keep homes, ABI return moves, register
  parsing/conversion, alias checks, scratch choice, scalar ALU record
  construction, and emission order target-local.

## Proof

No build run; delegated proof was metadata/analysis-only. No implementation or
test files were changed, and no `test_after.log` was required by the packet.
