# 176 Return-chain owner/schema decision

## Goal

Decide whether return-chain terminal/next-operand identity should become a
target-neutral BIR route, a target-local AArch64 owner, or an explicitly public
prepared API.

This idea is analysis-first. It should produce follow-up implementation ideas
only after the owner/schema decision is clear.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `### Return-Chain No-Route Follow-Up`

## Scope

- `PreparedReturnChainLookups`
- `prepared_return_chain_value_key(...)`
- `make_prepared_return_chain_lookups(...)`
- `find_prepared_return_chain_terminal_value(...)`
- `find_prepared_return_chain_next_operand_value(...)`
- AArch64 `alu.cpp` consumers that recover terminal and next operand homes for
  return-chain emission.

## Required Analysis

- Inventory return-chain producer/consumer semantics and current AArch64 use.
- Decide whether terminal and next-operand identity is target-neutral enough
  for BIR schema.
- If BIR-owned, define accepted/rejected cases, key stability rules, and
  boundaries against homes/registers/final instruction order.
- If target-local or prepared-owned, explain why future x86/riscv should not
  consume a BIR route for this relationship.
- Produce follow-up ideas for the chosen implementation route.

## Out Of Scope

- Folding return-chain into Route 1 producer identity or Route 7 comparison
  provenance without a new explicit route decision.
- Hiding the prepared return-chain helpers before owner/schema is settled.
- Moving homes, registers, ALU emission order, or final instruction records
  into BIR.

## Proof Route

Analysis-first. Any implementation follow-up should add oracle tests for
terminal and next-operand answers before API contraction.

## Reviewer Reject Signals

- Return-chain is justified by an unrelated route without accepted/rejected
  cases.
- Prepared helpers are hidden before consumers have a replacement owner.
- The route moves target-local ALU emission policy into BIR.
