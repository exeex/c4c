# 179 BIR return-chain consumer migration

## Goal

Migrate AArch64 return-chain consumers from prepared return-chain helper reads
to the BIR-owned return-chain route after oracle equivalence is proven.

## Source

Follow-up from `ideas/open/176_return_chain_owner_schema_decision.md`,
`ideas/open/177_bir_return_chain_schema_index.md`, and
`ideas/open/178_bir_return_chain_oracle_equivalence.md`.

## Scope

- AArch64 terminal return-chain register recovery currently using prepared
  terminal answers.
- AArch64 next-operand alias/scratch handling currently using prepared
  next-operand answers.
- Context projection needed for AArch64 to read the BIR-owned route.

## Required Work

- Replace semantic terminal and next-operand lookups with reads from the
  BIR-owned route.
- Keep target-local policy in AArch64: value homes, ABI return move lookup,
  register parsing/conversion, alias checks, scalar register view, scratch
  selection, ALU record construction, and emission order.
- Keep prepared helpers available as migration diagnostics or fallback only as
  explicitly justified by the implementation plan.
- Preserve fail-closed behavior when the BIR route has no answer or reports an
  invalid/conflicting answer.

## Out Of Scope

- Adding the BIR schema/index from scratch.
- Adding the initial oracle equivalence suite.
- Contracting the prepared API.
- Moving target policy into the BIR route or route facade.

## Proof Route

Run focused AArch64 return-chain lowering tests plus the oracle equivalence
suite from `ideas/open/178_bir_return_chain_oracle_equivalence.md`. Escalate to
broader backend validation if context projection, route facades, or public
lookup headers change.

## Reviewer Reject Signals

- Consumer migration depends on prepared answers without a bounded fallback
  removal plan.
- BIR records contain target homes, registers, scratch decisions, or final ALU
  records.
- Missing or conflicting BIR answers are handled by choosing target-local
  winners instead of failing closed.
- The migration weakens existing supported return-chain behavior.
