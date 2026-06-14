# Phase F3 Route 6 Call Identity Parity Blocker Map

Status: Active
Source Idea: ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md

## Purpose

Turn the Phase F2 Route 6 call-use/source boundary into an executable blocker
map. The result should prove one narrow x86/riscv parity path, or state exactly
why public prepared call-plan authority must remain blocked.

## Goal

Name the Route 6 call-use/source semantic fact, the bounded x86 consumer, and
either a bounded riscv consumer or explicit fail-closed riscv non-applicability.

## Core Rule

Do not delete, privatize, wrap, rename, or weaken public prepared call-plan
surfaces. This plan maps readiness only; prepared call-plan APIs remain
compatibility mirrors unless a later source idea explicitly authorizes an
implementation packet.

## Read First

- ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md
- ideas/closed/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md
- Existing call-plan, Route 6, wrapper, route-debug, and `ConsumedPlans`
  consumers in the current tree.

## Scope

- `PreparedFunctionLookups::call_plans`
- `PreparedBirModule::call_plans`
- `find_prepared_call_plans`
- Route 6 call-use/source identity evidence
- x86 direct-call, wrapper, route-debug, fallback, and `ConsumedPlans` output
- riscv call-related consumers or explicit riscv non-applicability proof
- missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  fallback, and policy-sensitive fail-closed behavior

## Non-Goals

- Do not implement a call-plan adapter.
- Do not move ABI call sequence, argument/result layout, stack/register policy,
  helper selection, wrapper instruction text, or public expected strings into
  BIR.
- Do not rewrite wrapper baselines, route-debug names, fallback names,
  helper/oracle statuses, or `ConsumedPlans` expected strings.
- Do not broaden this work into Route 6 migration beyond the call-use/source
  identity fact.

## Working Model

- Route/BIR may own the positive call-use/source semantic fact.
- Targets own ABI, register, stack, layout, helper choice, wrapper text, and
  emitted output policy.
- Public prepared call-plan APIs stay observable compatibility authority until
  agreement and fail-closed behavior are proven for the selected fact family.
- Compatibility rows are not proof of semantic ownership transfer by
  themselves.

## Execution Rules

- Keep evidence in `todo.md` unless the runbook itself needs correction.
- Prefer AST-backed symbol queries or precise text searches over broad manual
  scans.
- Treat expectation downgrades, named-case shortcuts, helper/status renames,
  and wrapper baseline rewrites as route-quality failures.
- If a new implementation initiative becomes concrete, record it as a separate
  `ideas/open/*.md` proposal instead of expanding this active plan.

## Step 1: Inventory Call-Plan Surfaces and Consumers

Goal: identify every public prepared call-plan surface and the current x86 and
riscv consumers relevant to Route 6 call-use/source identity.

Primary targets:

- `PreparedFunctionLookups::call_plans`
- `PreparedBirModule::call_plans`
- `find_prepared_call_plans`
- x86 direct-call, wrapper, route-debug, fallback, and `ConsumedPlans` paths
- riscv call-related lowering, diagnostics, or explicit absence points

Actions:

- Inspect declarations, definitions, and direct callers for the prepared
  call-plan surfaces.
- Separate semantic call-use/source consumers from compatibility-only string,
  wrapper, debug, fallback, and status consumers.
- Record whether each riscv bucket has direct consumption, indirect
  consumption, or no applicable call-plan path.

Completion check:

- `todo.md` contains a consumer inventory that distinguishes x86 semantic use,
  x86 compatibility output, riscv direct or indirect use, and riscv
  non-applicability candidates.

## Step 2: Name the Route 6 Semantic Fact

Goal: define the narrow call-use/source identity fact that could be owned by
Route 6 or BIR without absorbing target policy.

Actions:

- Identify the current route/BIR representation of the accepted scalar call
  evidence.
- Define the positive semantic fact in terms of caller/source/use identity,
  not ABI, wrapper text, register assignment, stack layout, helper choice, or
  emitted output.
- List prepared call-plan answers that must agree with the fact or fail closed.

Completion check:

- `todo.md` names one candidate semantic authority and the prepared agreement
  rows that must mirror it.

## Step 3: Build the x86 Agreement and Compatibility Matrix

Goal: prove or block one bounded x86 consumer for the selected Route 6
call-use/source fact.

Actions:

- Map direct-call and wrapper consumers against the selected semantic fact.
- Preserve `ConsumedPlans`, wrapper output, route-debug names, fallback names,
  helper/oracle statuses, and unsupported rows as compatibility output.
- Mark each x86 row as semantic agreement, retained compatibility, target
  policy, or blocked public prepared authority.

Completion check:

- `todo.md` states whether a bounded x86 consumer can read the selected fact
  while prepared call-plan output remains stable.

## Step 4: Build the riscv Applicability Matrix

Goal: prove a bounded riscv consumer for the same fact or document explicit
fail-closed non-applicability.

Actions:

- Inspect riscv call-related surfaces for direct or indirect dependence on
  call-use/source identity.
- If no riscv consumer applies, document the exact non-applicability reason
  and the fail-closed behavior that prevents accidental prepared-only use.
- Keep riscv target policy separate from the shared semantic fact.

Completion check:

- `todo.md` names the riscv consumer path or a concrete non-applicability
  proof, with policy boundaries preserved.

## Step 5: Define Fail-Closed and Mismatch Requirements

Goal: define the negative proof set required before prepared call-plan APIs can
act only as compatibility mirrors for this fact family.

Actions:

- Cover missing, invalid, duplicate/conflict, mismatch, unsupported,
  prepared-only, fallback, and policy-sensitive cases.
- Tie each negative case to the public prepared status, debug, fallback,
  wrapper, or `ConsumedPlans` surface that must remain stable.
- Reject any row that requires expectation weakening or baseline rewriting.

Completion check:

- `todo.md` contains a fail-closed checklist sufficient for a reviewer to
  reject overfit or compatibility-weakening routes.

## Step 6: Decide Adapter Readiness or Blocker Status

Goal: conclude whether a later one-adapter implementation idea is safe, or the
Route 6 call identity boundary remains blocked.

Actions:

- Compare the x86 matrix, riscv matrix, and fail-closed checklist against the
  source idea acceptance criteria.
- If one bounded implementation packet is safe, specify its exact source-idea
  shape without implementing it.
- If not safe, record the blocker reason and retained public prepared
  authority.

Completion check:

- `todo.md` records the final readiness decision and any proposed follow-up
  idea scope. No implementation files or baselines are changed.
