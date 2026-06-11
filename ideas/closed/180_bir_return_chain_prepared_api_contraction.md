# 180 BIR return-chain prepared API contraction

## Goal

Contract the prepared return-chain API after the BIR-owned route is proven and
target consumers no longer require public prepared return-chain helpers.

## Source

Follow-up from `ideas/open/176_return_chain_owner_schema_decision.md`,
`ideas/open/178_bir_return_chain_oracle_equivalence.md`, and
`ideas/open/179_bir_return_chain_consumer_migration.md`.

## Scope

- Public prepared return-chain helper visibility.
- Prepared lookup aggregate fields that exist only to expose return-chain
  terminal or next-operand identity.
- Tests or diagnostics that still need a migration oracle.

## Required Work

- Confirm BIR route/schema tests and prepared/BIR oracle equivalence are green.
- Confirm AArch64 consumers read the BIR-owned route for semantic terminal and
  next-operand answers.
- Remove, hide, or narrow prepared return-chain helpers only after no active
  consumer needs them as public API.
- Preserve any internal prepared implementation detail only when another
  bounded route still owns it.

## Out Of Scope

- Adding the BIR schema/index.
- Adding the initial oracle equivalence suite.
- Migrating AArch64 consumers.
- Changing return-chain semantics or broadening accepted cases.

## Proof Route

Run the oracle equivalence suite, focused AArch64 return-chain lowering tests,
and any prepared lookup API tests touched by the contraction. Escalate to
broader backend validation when public headers or aggregate projection shape
changes.

## Reviewer Reject Signals

- Prepared helpers are hidden before BIR/prepared equivalence and consumer
  migration are proven.
- The contraction changes accepted/rejected return-chain semantics.
- Tests are weakened or deleted instead of being retargeted to the BIR route.
- Public API cleanup is bundled with new schema behavior or consumer migration.

## Closure

Closed after the prepared return-chain API contraction runbook completed.
The public prepared return-chain helper surface was removed from
`src/backend/prealloc/prepared_lookups.hpp`; removed names had no remaining
matches under `src` or `tests`; focused return-chain/prepared helper proof and
the broader `^backend_` subset were green, with 180/180 backend tests passing.
