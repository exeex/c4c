Status: Active
Source Idea Path: ideas/open/176_return_chain_owner_schema_decision.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Return-Chain Semantics

# Current Packet

## Just Finished

Completed Step 1 - Inventory Return-Chain Semantics. Recorded the prepared
return-chain producer contract, accepted and fail-closed shapes, AArch64
terminal/next-operand consumer expectations, and target-policy boundaries in
`docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`.

## Suggested Next

Proceed to Step 2 by deciding whether terminal/next-operand identity should be
BIR-owned, target-local AArch64-owned, or explicitly public prepared-owned,
using the Step 1 facts as the boundary checklist.

## Watchouts

- The current prepared lookup owns only target-neutral identity keyed by block
  index, instruction index, and chain value name; AArch64 owns homes, ABI return
  moves, register parsing/conversion, alias checks, scratch choice, scalar ALU
  record construction, and emission order.
- Producer acceptance is same-block, named-return, named-result scalar binary
  chains that reach the return terminator; broken, unnamed, unsupported, or
  conflicting cases fail closed.

## Proof

No build run; delegated proof was metadata/analysis-only and no implementation
or test files were changed.
