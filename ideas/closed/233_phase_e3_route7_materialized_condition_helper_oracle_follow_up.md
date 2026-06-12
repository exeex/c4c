# 233 Phase E3 Route 7 materialized-condition helper-oracle follow-up

## Goal

Augment or replace one Route 7 materialized-condition helper-oracle row in
`verify_prepared_bir_comparison_condition_producer_equivalence` with Route 7
comparison evidence under prepared agreement.

## Why This Exists

The Route 7 comparison closure showed that Route 7-native comparison evidence
can augment one materialized-condition helper-oracle row. Phase E3 accepted
that row for a later follow-up while retaining prepared oracle assertion
strength, branch policy, branch-control output, wrappers, final assembler
behavior, helper-oracle strings, and expected strings.

## In Scope

- One materialized-condition helper-oracle row in
  `verify_prepared_bir_comparison_condition_producer_equivalence`.
- Positive Route 7 comparison evidence under prepared agreement.
- Absent-route, invalid-reference, duplicate/conflict, mismatch, unfused,
  prepared fallback, and non-agreement cases.
- Stable oracle assertion strength, branch-control output, wrappers, final
  assembler behavior, helper-oracle strings, expected strings, and nearby
  same-feature coverage.

## Out Of Scope

- Route 7-wide comparison migration, branch-control replacement,
  machine-printer replacement, route-index migration, or branch policy movement.
- Suffix mapping, fused legality, hazard checks, final assembler policy,
  wrapper migration, or emitted-output authority changes.
- Expected-string rewrites, baseline refreshes, helper renames, unsupported
  downgrades, or weakened oracle behavior.
- Draft 155, E5, aggregate prepared retirement, or broad prepared
  diagnostic/oracle replacement.

## Acceptance Criteria

- The materialized-condition row uses Route 7 comparison evidence only under
  prepared agreement.
- Absent-route, invalid-reference, duplicate/conflict, mismatch, unfused,
  prepared fallback, and non-agreement paths retain prepared oracle authority.
- Oracle assertion strength, branch-control output, wrappers, final assembler
  behavior, helper-oracle strings, and expected strings remain unchanged.
- Proof covers nearby same-feature materialized-condition and fused-compare
  cases without promoting branch-control or machine-printer ownership.
- No route-index, wrapper, baseline, expected-string, or target-policy
  migration is part of accepted progress.

## Reviewer Reject Signals

- The implementation adds testcase-shaped matching for one materialized
  condition instead of using Route 7 comparison evidence under agreement.
- It weakens prepared oracle assertion strength or fallback for absent-route,
  invalid-reference, duplicate/conflict, mismatch, unfused, or non-agreement
  cases.
- It changes branch-control output, wrappers, final assembler behavior,
  helper-oracle strings, expected strings, or baselines.
- It broadens into Route 7-wide comparison, branch-control, machine-printer, or
  route-index migration.
- It renames the old prepared failure/success mode without proving the Route 7
  materialized-condition fact.

## Closure Notes

Closed after the four-step runbook completed:

- Step 1 discovery: `fe4e4698e`
- Step 2 helper-oracle agreement assertions: `ac8211459`
- Step 3 stability proof: `5c8314dc5`
- Step 4 acceptance notes: `0efc021fb`

The selected materialized-condition helper-oracle row now uses Route 7
comparison evidence only under prepared agreement. Prepared oracle authority
remains retained for absent-route, invalid-reference, duplicate/conflict,
mismatch, unfused, prepared fallback, and non-agreement paths. Helper-oracle
strings, branch-control output, wrappers, final assembler behavior, expected
strings, baselines, route-index public contracts, and supported/unsupported
contracts remain out of scope and unchanged.

Close proof used matching before/after logs for:
`backend_prepared_lookup_helper`,
`backend_aarch64_branch_control_lowering`, and
`backend_aarch64_instruction_dispatch`.
The regenerated `test_after.log` passed all three tests and the regression
guard passed against `test_before.log` with non-decreasing pass count allowed
for the rolled-forward accepted baseline.
