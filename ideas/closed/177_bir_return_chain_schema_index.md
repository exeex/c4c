# 177 BIR return-chain schema/index

## Goal

Add the BIR-owned return-chain record and function-local lookup/index for
terminal return value and immediate next-operand identity.

## Source

Follow-up from `ideas/open/176_return_chain_owner_schema_decision.md` and
`docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`.

## Scope

- BIR route/schema representation for same-block return-chain identity.
- Function-local index keyed by function, block, instruction position or
  stable instruction reference, and chain value.
- Construction or invalidation rules for accepted, rejected, ambiguous, and
  conflicting return-chain records.

## Required Work

- Add a distinct BIR return-chain route; do not fold it into Route 1 producer
  identity or Route 7 comparison provenance.
- Store only target-neutral identity: block, instruction reference or index,
  chain value, terminal return value, and optional immediate next-operand
  value.
- Reject or omit answers for unsupported opcodes, unnamed values, broken
  same-block walks, non-return terminators, cross-block relationships, and
  conflicting duplicate publications.
- Preserve cheap lookup by the same logical key used by the current prepared
  helpers.

## Out Of Scope

- Migrating AArch64 consumers.
- Hiding or privatizing prepared return-chain helpers.
- Moving homes, registers, ABI return placement, alias checks, scratch choice,
  ALU record construction, or final emission order into BIR.

## Proof Route

Schema/index tests should prove that records are created or omitted for focused
positive, negative, ambiguous, and conflict fixtures. Full prepared equivalence
belongs to `ideas/open/178_bir_return_chain_oracle_equivalence.md`.

## Completion Note

Closed after the Route 8 schema/index runbook completed through Step 5.
The implemented route records target-neutral return-chain identity, exposes a
function-local lookup/index, fails closed for unsupported and conflicting cases,
and leaves prepared helpers public with AArch64 consumers unmigrated. The next
prerequisite remains oracle-equivalence coverage in
`ideas/open/178_bir_return_chain_oracle_equivalence.md`.

## Reviewer Reject Signals

- The route imports target homes, registers, ABI moves, scratch policy, or
  emitted instruction order.
- The implementation chooses a winner for conflicting duplicate answers.
- The key is unstable across the BIR block being indexed without rebuild or
  invalidation.
- The slice hides or weakens the prepared helpers before oracle equivalence is
  available.
