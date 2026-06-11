# 197 Return-chain import and naming clarification

## Goal

Import the completed return-chain owner/schema result as a distinct route line
for future retirement analysis and clarify how future plans should cite it.

Return-chain must remain separate from Route 1 producer identity, Route 7
comparison provenance, predecessor rescans, name matching, and generic
route-index facades.

## Why This Exists

The pre-Phase-E readiness audit concludes that return-chain is importable from
`docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md` and closed
ideas 176-180, but only as its own owner/schema line. Folding it into existing
route families would erase the boundary between target-neutral chain identity
and AArch64 return ABI, value-home, register, alias, scratch, and emission
policy.

## In Scope

- Summarize the closed return-chain owner/schema result in a form future Phase
  E retirement analysis can cite directly.
- Define the durable name for the return-chain route or owner/schema line.
- State which facts are target-neutral return-chain identity.
- State which facts remain AArch64 or prepared-owned policy.
- Identify any residual prepared return-chain helpers that remain public
  migration oracles.

## Out Of Scope

- Reopening or reimplementing closed return-chain ideas 176-180.
- Folding return-chain into Route 1, Route 7, predecessor rescan logic, name
  matching, or a generic route-index facade.
- Deleting prepared return-chain helpers without equivalent route-owned
  positive, negative, ambiguous, and conflict coverage.
- Changing AArch64 return ABI or emission behavior.
- Opening draft 155.

## Acceptance Criteria

- A durable return-chain import note names the source documents and closed
  ideas that future retirement analysis should cite.
- The accepted target-neutral facts are listed separately from AArch64 or
  prepared-owned policy.
- Future plans have a stable name for the return-chain owner/schema line.
- Residual prepared return-chain oracle/fallback helpers are named or
  explicitly declared absent.
- The note explains how return-chain should appear in `PreparedFunctionLookups`
  and `PreparedBirModule` readiness analysis without merging into Routes 1 or
  7.

## Reviewer Reject Signals

- Folding return-chain into Route 1 producer identity, Route 7 comparison
  provenance, predecessor rescans, name matching, or a generic route-index
  facade.
- Claiming return-chain retirement readiness without importing the dedicated
  owner/schema decision and ideas 176-180.
- Moving AArch64 return ABI, register selection, value homes, alias detection,
  scratch selection, final ALU records, or emission order into target-neutral
  return-chain facts.
- Deleting prepared return-chain helpers without equivalent positive,
  negative, ambiguous, and conflict coverage.
- Claiming progress through naming-only changes while leaving the import
  ambiguity unresolved.

## Completion Note

Closed after creating
`docs/bir_prealloc_fusion/return_chain_import_and_naming.md`. The note imports
the return-chain owner/schema result as `Route 8 return-chain owner/schema
line`, cites the decision doc and closed ideas 176-180, separates
target-neutral BIR return-chain identity from target-local policy, and records
the old public prepared return-chain helper names as historical/absent after
idea 180.
