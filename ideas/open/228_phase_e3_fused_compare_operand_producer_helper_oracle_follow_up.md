# 228 Phase E3 fused-compare operand-producer helper-oracle follow-up

## Goal

Augment or replace one selected
`find_prepared_fused_compare_operand_producer_facts(...)` helper-oracle
operand-producer row family with Route 7 comparison provenance evidence when
the private Route 7 agreement reader agrees with prepared data.

## Why This Exists

Phase E1 proved Route 7/prepared agreement for the selected comparison
provenance identity read consumed by AArch64 comparison lowering. Phase E2
extracted the private Route 7 agreement reader for that selected fused-compare
operand-producer fact. Phase E3 classified the matching helper-oracle positive
row family as ready while retaining public prepared fallback, branch-control,
machine-printer, wrapper, and expected-string authority.

## In Scope

- One selected fused-compare operand-producer helper-oracle row family around
  the private Route 7 agreement reader.
- Positive Route 7 comparison provenance under prepared agreement.
- Fail-closed behavior for absent route, invalid references,
  duplicate/conflict, mismatch, unfused or non-agreement paths, and prepared-only
  fallback.
- Proof that helper-oracle status/name/string behavior, branch-control output,
  machine-printer output, wrappers, expected strings, and nearby same-feature
  cases remain stable.

## Out Of Scope

- Broad Route 7 migration, aggregate route view replacement, prepared aggregate
  retirement, branch-control replacement, machine-printer replacement, or target
  branch policy movement.
- Suffix mapping, fused legality, hazard checks, final assembler rows, wrapper
  output, or expected-output policy changes.
- Baseline refreshes, expected-string rewrites, helper renames, unsupported
  downgrades, or weakened oracle authority.
- Draft 155, E5, broad prepared diagnostic/oracle replacement, or broad route
  facade migration.

## Acceptance Criteria

- The positive helper-oracle row uses Route 7 comparison provenance only after
  prepared agreement.
- Absent route, invalid reference, duplicate/conflict, mismatch, unfused,
  non-agreement, and prepared-only paths keep prepared fallback/oracle
  authority.
- Branch-control and machine-printer output remain byte-stable.
- Proof covers nearby same-feature fused-compare cases, including fallback
  cases, without expectation rewrites.
- No aggregate route view, target policy, wrapper, baseline, or expected-string
  change is part of the accepted progress.

## Reviewer Reject Signals

- The route adds named-case matching for one fused-compare testcase instead of
  using the private Route 7 agreement reader.
- It weakens fallback for absent route, invalid reference, duplicate/conflict,
  mismatch, unfused, non-agreement, or prepared-only cases.
- It presents branch-control, machine-printer, suffix, fused-legality, hazard,
  wrapper, or final assembler changes as part of this helper-oracle slice.
- It claims progress through expected-string rewrites, helper renames,
  unsupported downgrades, or baseline refreshes.
- It keeps the prepared-only oracle result under a new abstraction name without
  consuming the proven Route 7 provenance fact.
