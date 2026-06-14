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

## Closure Evidence

Closed after the active runbook completed Steps 1-6. The blocker map is
complete as an analysis result, not as an implementation change.

The mapped Route 6 semantic fact is scalar `i32` call argument source identity
after Route 6 and prepared `source_value_id` agreement. The only bounded
implementation-ready consumer is x86-local and must pass all recorded
agreement gates before using the fact.

Public prepared call-plan APIs remain compatibility authority:

- `PreparedFunctionLookups::call_plans`
- `PreparedBirModule::call_plans`
- `find_prepared_call_plans`

They must remain public and observable. `ConsumedPlans`, wrapper output,
route-debug names, fallback names, helper/oracle statuses, unsupported rows,
and target ABI/register/stack/result policy remain compatibility or target
authority, not Route 6 semantic ownership.

The riscv side is explicitly non-applicable for this fact family in the
current tree. No bounded riscv consumer currently reads the selected Route 6
call-use/source identity; ordinary riscv call lowering uses generic operands
and target-local call policy, while prepared riscv module emission consumes
edge-publication agreement rather than Route 6 call-plan identity.

Broad x86/riscv parity and public prepared call-plan authority demotion remain
blocked. The implementation-ready remainder has been split into
`ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md`.
