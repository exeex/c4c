# 178 BIR return-chain oracle equivalence

## Goal

Prove the new BIR-owned return-chain route is equivalent to the existing
prepared return-chain helpers before any consumer migration or API contraction.

## Source

Follow-up from `ideas/open/176_return_chain_owner_schema_decision.md`,
`docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`, and
`ideas/open/177_bir_return_chain_schema_index.md`.

## Scope

- Oracle tests comparing BIR route answers with
  `find_prepared_return_chain_terminal_value(...)`.
- Oracle tests comparing BIR route answers with
  `find_prepared_return_chain_next_operand_value(...)`.
- Positive, negative, ambiguous, and conflict fixtures for terminal and
  next-operand answers.

## Required Work

- Build both answer sets for the same function, block, instruction position,
  and chain value.
- Assert exact equivalence for accepted terminal answers and accepted
  next-operand answers.
- Assert no usable answer for rejected and ambiguous cases.
- Assert fail-closed behavior for duplicate publications with conflicting
  terminal or next-operand answers.
- Keep prepared helpers public as the oracle during this work.

## Out Of Scope

- Changing AArch64 consumers to prefer the BIR route.
- Contracting or hiding the prepared return-chain API.
- Broadening the route beyond same-function, same-block scalar binary chains
  ending at a BIR return terminator.

## Proof Route

Run the focused oracle suite plus any existing prepared lookup tests that cover
return-chain terminal and next-operand helpers. Escalate if public headers,
`PreparedFunctionLookups`, route facades, or AArch64 context projection are
changed.

## Reviewer Reject Signals

- Tests compare only positive happy paths.
- Negative, ambiguous, or conflict cases are represented by expectation
  downgrades instead of semantic fail-closed assertions.
- The BIR route is treated as correct without comparing against the prepared
  oracle.
- Prepared helpers are hidden before the oracle suite is green.
