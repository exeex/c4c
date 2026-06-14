# 249 Phase F3 Route 6 call identity parity blocker map

## Idea Type

x86/riscv parity proof.

## Goal

Prove or explicitly block the narrow Route 6 call-use/source identity boundary
needed before `PreparedFunctionLookups::call_plans`,
`PreparedBirModule::call_plans`, or `find_prepared_call_plans` can stop being
public prepared semantic authority.

## Why This Exists

Phase F2 found that selected x86 scalar `i32` Route 6 evidence is useful but
not broad call-plan, wrapper, or riscv readiness evidence. Public prepared call
plan answers still own observable compatibility for `ConsumedPlans`, wrapper
output, route-debug names, fallback names, helper/oracle statuses, and
unsupported fail-closed rows.

## In Scope

- Inventory the exact x86 direct-call and wrapper consumers that need the
  Route 6 call-use/source fact.
- Inventory riscv consumers or prove explicit riscv non-applicability for the
  same fact family.
- Define route/prepared agreement and mismatch checks for missing, invalid,
  duplicate/conflict, unsupported, prepared-only, fallback, and
  policy-sensitive cases.
- Preserve ABI call sequence, argument/result layout, stack/register policy,
  helper selection, wrapper instruction text, and public expected strings as
  target policy or compatibility output.

## Out Of Scope

- Deleting or privatizing `call_plans`.
- Rewriting wrapper baselines or `ConsumedPlans` expected strings.
- Moving ABI, register, stack, layout, helper choice, or wrapper output policy
  into BIR.
- Broad Route 6 migration outside the call-use/source identity fact.

## Acceptance Criteria

- A future packet names one Route 6 call-use/source semantic authority and one
  bounded x86 consumer plus either one bounded riscv consumer or explicit
  fail-closed riscv non-applicability.
- Prepared call-plan APIs remain compatibility mirrors and continue to expose
  stable statuses, debug names, fallback names, and wrapper output rows.
- Mismatch and unsupported cases fail closed without expectation weakening.
- The result states whether a later one-adapter implementation idea is ready
  or this boundary remains blocked.

## Reviewer Reject Signals

- Reject named-case shortcuts such as handling only the previously accepted
  x86 scalar `i32` row while claiming direct-call, wrapper, or riscv parity.
- Reject unsupported expectation downgrades, weaker `ConsumedPlans`
  contracts, or wrapper-baseline rewrites without explicit approval.
- Reject helper renames, route-debug text rewrites, or classification-only
  notes claimed as capability progress.
- Reject broad Route 6, call lowering, ABI, register, stack, or wrapper
  rewrites outside the call-use/source identity boundary.
- Reject any route that leaves prepared call-plan lookup data as the exact old
  semantic source while renaming the access path as route/BIR-owned.
