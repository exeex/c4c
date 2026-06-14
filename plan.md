# Phase F3 x86 Route 6 Call Argument Source Identity Adapter

Status: Active
Source Idea: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md

## Purpose

Implement one x86-local adapter for scalar named-`i32` call argument source
identity after Route 6 and prepared `source_value_id` agreement.

## Goal

Add a narrow x86 agreement path that only treats call argument source identity
as semantic authority when Route 6 call-use/source evidence and prepared
`source_value_id` identify the same scalar named-`i32` value.

## Core Rule

Keep prepared call-plan APIs and compatibility output public and stable. The
adapter may claim semantic source identity only after both Route 6 evidence and
prepared `source_value_id` agreement pass; every missing, unsupported,
duplicate/conflict, mismatch, fallback, prepared-only, or Route 6-only row must
fail closed.

## Read First

- Source idea:
  `ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md`
- x86 call argument planning and Route 6 call-use/source records.
- Prepared call-plan lookup surfaces:
  `PreparedFunctionLookups::call_plans`,
  `PreparedBirModule::call_plans`, and `find_prepared_call_plans`.
- Focused backend tests for prepared call plans, route-debug output,
  wrapper/fallback behavior, helper/oracle status, and `ConsumedPlans`.

## Current Scope

In scope:

- Identify the existing x86 scalar named-`i32` call argument source identity
  consumer and the Route 6/prepared fields it can compare.
- Add one x86-local agreement adapter or query helper for that fact.
- Preserve prepared call-plan API observability and exact compatibility output.
- Add focused proof for the agreeing path and fail-closed rejection rows.

Non-goals:

- RISC-V implementation or parity claims.
- Deleting, privatizing, renaming, or bypassing public prepared call-plan APIs.
- Moving ABI call sequence, argument/result layout, register/stack policy,
  helper selection, wrapper instruction text, fallback names, route-debug
  strings, or expected output into Route 6 or BIR.
- Broad Route 6 migration beyond the selected scalar call argument
  source-identity fact.

## Working Model

- Route 6 provides candidate call-use/source evidence, but this runbook only
  trusts it for the selected x86 scalar named-`i32` fact when the prepared
  `source_value_id` row agrees.
- Prepared call-plan lookup/status remains the retained compatibility surface.
- The adapter is not a replacement for ABI, register, stack, wrapper, helper,
  or fallback policy. Those decisions stay target-owned and compatibility
  guarded.
- Any proof gap in unsupported, mismatch, duplicate/conflict, fallback, or
  one-sided evidence behavior blocks acceptance.

## Execution Rules

- Keep each step limited to the selected x86 scalar named-`i32` call argument
  source identity.
- Prefer existing helpers and local backend patterns before introducing a new
  abstraction.
- Do not weaken expected strings, helper/oracle statuses, route-debug output,
  fallback names, wrapper baselines, or `ConsumedPlans` contracts.
- Treat prepared-only evidence and Route 6-only evidence as non-agreement.
- Record any adjacent riscv or broader Route 6 work in `todo.md` as out of
  scope, not as part of this implementation.

## Steps

### Step 1: Call Argument Source Inventory

Goal: locate the exact x86 call argument source identity reader and the Route 6
and prepared facts available for agreement.

Primary targets:

- x86 call argument planning and call-use consumption
- Route 6 call-use/source identity records
- prepared call-plan `source_value_id` rows
- tests for prepared call plans, route-debug, wrappers, helper/oracle status,
  fallback behavior, and `ConsumedPlans`

Actions:

- Search for x86 call argument source/value planning and prepared call-plan
  lookup reads.
- Identify the scalar named-`i32` row that can be safely isolated.
- Record the Route 6 fact, prepared fact, retained compatibility surface, and
  nearby fail-closed cases in `todo.md`.

Completion check:

- `todo.md` names the reader/helper row, the matching Route 6 and prepared
  fields, and the proof rows needed before code changes.

### Step 2: Agreement Adapter Design

Goal: define the smallest x86-local adapter or query helper that checks Route 6
and prepared `source_value_id` agreement.

Actions:

- Choose the local helper/function boundary that serves the selected x86
  consumer without exposing a broad public API.
- Specify exact rejection behavior for missing, invalid, duplicate/conflict,
  mismatch, unsupported, prepared-only, Route 6-only, fallback, and
  policy-sensitive rows.
- Confirm public prepared call-plan APIs remain observable and unchanged.

Completion check:

- `todo.md` describes the adapter boundary, accepted agreement row, rejection
  matrix, and unchanged compatibility surfaces.

### Step 3: Implement x86 Agreement Path

Goal: add the adapter and route the selected x86 scalar named-`i32` call
argument source identity through it.

Actions:

- Implement the local agreement check using existing Route 6 and prepared
  call-plan data.
- Fail closed for every non-agreeing or ambiguous row without changing ABI,
  wrapper, helper, fallback, route-debug, or output policy.
- Keep the implementation narrow enough that riscv and broad Route 6 behavior
  are untouched.

Completion check:

- The selected x86 consumer uses the agreement adapter, and nearby behavior is
  unchanged when agreement is absent.

### Step 4: Focused Proof

Goal: prove the adapter accepts only the agreed scalar named-`i32` row and
preserves existing compatibility behavior.

Actions:

- Add or update focused backend tests for the agreeing x86 path.
- Cover prepared-only, Route 6-only, mismatch, unsupported,
  duplicate/conflict, fallback, and policy-sensitive rejection rows where the
  local test surface supports them.
- Re-run the delegated narrow build/test command and record the result in
  `test_after.log`.

Completion check:

- Fresh proof passes and `todo.md` records the exact command and result.

### Step 5: Compatibility Sweep

Goal: verify the implementation did not weaken prepared call-plan, route-debug,
wrapper, helper/oracle, fallback, or `ConsumedPlans` behavior.

Actions:

- Re-check public prepared call-plan APIs and compatibility tests named during
  Step 1.
- Confirm riscv remains non-applicable and unchanged.
- Record any broader validation recommendation for the supervisor.

Completion check:

- The source idea acceptance criteria are satisfied: the x86 adapter is narrow,
  agreement-gated, fail-closed, compatibility-preserving, and proved by focused
  backend tests.
