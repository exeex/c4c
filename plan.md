# AArch64 Direct-Call Argument And Formal Publication Runbook

Status: Active
Source Idea: ideas/open/346_aarch64_direct_call_argument_formal_publication.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Move the post-345 backend-regex inventory out of the umbrella and into a
focused direct-call ABI publication owner before implementation work begins.

## Goal

Repair the AArch64 direct-call path so prepared call operands and callee
formals publish to and consume the AAPCS64 ABI argument registers and stack
slots for scalar, FP, address or aggregate, and overflow/stack cases.

## Core Rule

Progress must repair direct-call argument/formal publication semantics. Do not
claim progress through filename matching, stale-register shortcuts,
expectation changes, unsupported classifications, runner changes, timeout
policy, proof-log policy, or CTest registration changes.

## Read First

- `ideas/open/346_aarch64_direct_call_argument_formal_publication.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `ideas/closed/309_aarch64_indirect_call_argument_preservation.md`
- `ideas/closed/311_aarch64_selected_call_boundary_move_preparation_printing.md`
- `ideas/closed/336_aarch64_return_result_publication_epilogue_clobber.md`
- `ideas/closed/337_aarch64_callee_saved_scalar_home_preservation.md`
- `ideas/closed/345_aarch64_scalar_select_result_publication.md`
- Current generated artifacts under
  `build/c_testsuite_aarch64_backend/src/*.c.s`
- Prepared call/formal, ABI classification, and AArch64 call-lowering tests

## Current Targets

- Representative c-testsuite cases:
  - `c_testsuite_aarch64_backend_src_00140_c`
  - `c_testsuite_aarch64_backend_src_00159_c`
  - `c_testsuite_aarch64_backend_src_00170_c`
  - `c_testsuite_aarch64_backend_src_00175_c`
  - `c_testsuite_aarch64_backend_src_00218_c`
- Initial proof scope should include focused backend call/formal lowering
  tests plus the representative c-testsuite cases.
- Current inventory basis:
  - `/workspaces/c4c/test_after.log`
  - 354 selected, 330 passed, 24 failed, 2 timed out

## Non-Goals

- Do not reopen closed indirect-call, selected call-boundary printer,
  return-result publication, callee-saved scalar-home, or scalar-select owners
  from counts or filename recurrence alone.
- Do not absorb pointer/null condition publication, FP comparison
  materialization, aggregate/table memory corruption, libc/file/string
  residuals, semantic `lir_to_bir` admission, or timeout/output-storm buckets.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, or CTest registration.
- Do not fix only one stale register, one function name, one argument index,
  one stack offset, one c-testsuite filename, or one emitted instruction
  sequence.

## Working Model

The representative failures share a call-boundary ABI publication problem:
prepared direct-call operands and callee formal homes are not consistently
connected to the AAPCS64 argument registers and overflow stack slots used by
the generated callsite and function entry code.

Expected localization should distinguish caller-side operand publication from
callee-side formal consumption, and register arguments from overflow/variadic
stack arguments. It should preserve closed boundaries unless fresh evidence
shows the first bad fact moved back to one of them.

## Execution Rules

- Keep packet progress, exact commands, and residual notes in `todo.md`.
- Start with generated-code and prepared-state localization before editing
  backend implementation files.
- Add focused backend coverage before or with the semantic repair when a
  narrow unit-level contract can pin the direct-call publication rule.
- Treat c-testsuite count changes as supporting evidence, not as the primary
  proof of capability repair.
- Reclassify any remaining representative failure by first bad fact before
  asking to close this idea.

## Steps

### Step 1: Localize Direct-Call ABI Publication Facts

Goal: identify where each representative loses the prepared direct-call
argument or callee formal value.

Primary target: generated AArch64 assembly, prepared call/formal records, ABI
classification records, and existing focused backend tests.

Actions:

- Inspect `00140`, `00159`, `00170`, `00175`, and `00218` generated assembly
  and any prepared dumps needed to tie source arguments/formals to ABI
  locations.
- Separate caller-side operand publication gaps from callee-side formal
  consumption gaps.
- Separate register arguments, FP arguments, address or aggregate arguments,
  and overflow/variadic stack arguments.
- Compare the first bad facts against closed ideas 309, 311, 336, 337, and
  345 before selecting an implementation surface.
- Record localization results and the first implementation target in
  `todo.md`.

Completion check:

- `todo.md` names the concrete direct-call publication/consumption boundary
  to repair first and records why the representative cases belong to this
  owner instead of a closed or parked bucket.

### Step 2: Repair The Narrow Publication Boundary

Goal: repair the localized direct-call argument or formal publication rule
without broad call-lowering rewrites.

Primary target: the implementation surface identified in Step 1.

Actions:

- Add focused backend coverage for the localized direct-call publication or
  formal-consumption contract.
- Repair the general rule that maps prepared direct-call values to ABI
  argument locations or callee formal homes.
- Preserve indirect-call, selected call-boundary printer, return publication,
  callee-saved, and scalar-select guardrails.
- Run the delegated focused build/proof command and record results in
  `todo.md`.

Completion check:

- Focused coverage proves the repaired direct-call publication rule, and the
  delegated representative subset advances or passes without introducing a
  closed-owner regression.

### Step 3: Cover Remaining Representative Shapes

Goal: extend the repair only across the direct-call ABI shapes proven by the
representatives.

Primary target: scalar integer, FP, address or aggregate, and overflow/stack
direct-call argument/formal paths.

Actions:

- Re-run or inspect the representative set after Step 2.
- If a representative still fails with the same direct-call publication
  family, localize and repair that subcase under this idea.
- Keep unrelated residuals parked and record new first bad facts instead of
  broadening the owner.
- Add or update focused coverage for each newly repaired direct-call shape.

Completion check:

- `00140`, `00159`, `00170`, `00175`, and `00218` either pass or have
  remaining failures classified outside the direct-call publication first bad
  fact.

### Step 4: Validate And Reclassify For Closure

Goal: prove the focused owner is complete or hand off remaining residuals to
the umbrella inventory.

Primary target: focused direct-call proof plus supervisor-selected broader
backend validation.

Actions:

- Run the supervisor-approved focused proof and any requested broader backend
  subset.
- Compare before/after results through the repo's regression-guard workflow
  when closure is requested.
- Record any remaining representative first bad facts in `todo.md`.
- If the direct-call owner is complete, ask plan-owner to close or park this
  idea according to the regression-guard result and residual classification.

Completion check:

- The direct-call argument/formal publication owner is either closure-ready
  with proof, or `todo.md` records the exact blocker or next lifecycle split.
