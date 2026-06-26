# RV64 Frame-Slot Address Call Argument Runbook

Status: Active
Source Idea: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md
Activated from: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md

## Purpose

Turn the exposed RV64 object-route `unsupported_instruction_fragment` boundary
for same-module scalar GPR call arguments sourced from local frame-slot
addresses into a precise semantic lowering target or a narrower fail-closed
diagnostic.

## Goal

Support, or precisely route as unsupported, the prepared call-argument family
where a pointer argument is selected as
`PreparedCallArgumentSourceSelectionKind::FrameSlotAddress` for an
address-exposed local frame slot.

## Core Rule

Use prepared call-plan facts and BIR call semantics as the source of truth. Do
not infer behavior from testcase names, raw source syntax, or expectation
rewrites.

## Read First

- `ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md`
- `todo.md` Step 1 evidence for
  `tests/c/external/gcc_torture/src/va-arg-13.c`
- Existing RV64 object-route call lowering diagnostics for
  `unsupported_instruction_fragment`
- Prepared/BIR dump tooling for
  `tests/c/external/gcc_torture/src/va-arg-13.c`

## Current Targets

- `tests/c/external/gcc_torture/src/va-arg-13.c`
- Same-module `bir::CallInst` with a scalar GPR pointer argument
- `PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`
- Address-exposed local frame slot call-argument materialization

## Split-Out Family

`tests/c/external/gcc_torture/src/920908-1.c` exposed a separate same-module
ABI memory-return/sret call family rejected at the
`call_plan->memory_return.has_value()` admission gate. That owner is split into
`ideas/open/387_rv64_object_route_same_module_sret_calls.md` and is out of
scope for this active runbook unless a later reviewer proves both routes share
an existing common call-address materialization abstraction.

## Non-Goals

- Do not reopen scalar GPR stack-slot formal-home support from idea 374.
- Do not implement same-module memory-return/sret calls; use idea 387 for that
  family.
- Do not implement byval aggregate parameter homes or aggregate `va_arg`
  helper lowering.
- Do not reconstruct ABI placement, stack layout, or source intent from
  testcase names or raw syntax.
- Do not perform broad RV64 object-route rewrites unrelated to scalar pointer
  call arguments sourced from local frame-slot addresses.
- Do not downgrade expectations or convert supported-path coverage to
  unsupported coverage without explicit approval.

## Working Model

Step 1 evidence showed the active representative fails in
`fragment_for_prepared_call` after wrapper acceptance because the GPR argument
source selection is `FrameSlotAddress`. The missing capability is not generic
parameter-home support and not the same sret admission-gate failure tracked by
idea 387.

## Execution Rules

- Keep each packet tied to one plan step and update `todo.md` with proof.
- Prefer semantic lowering by BIR call kind, operand, type, storage, and
  prepared object-route call facts.
- Preserve precise fail-closed diagnostics for unsupported call families.
- Add focused backend coverage before treating `va-arg-13.c` as proof.
- Rerun `va-arg-13.c` after any lowering or diagnostic change.
- If the implementation reaches a later boundary, document it in `todo.md` and
  route that boundary instead of silently expanding this plan.

## Ordered Steps

### Step 1: Preserve Captured Call-Argument Evidence

Goal: keep the completed Step 1 evidence available as the factual base for
implementation.

Primary target:

- `todo.md` Step 1 evidence for the first failing `dummy(ptr %t7)` call in
  `va-arg-13.c`

Actions:

- Treat the existing `todo.md` Step 1 packet as the captured evidence record.
- Do not rerun diagnostics unless the executor needs a fresh local proof before
  changing code.
- Keep the split-out `920908-1.c` sret evidence routed to idea 387.

Completion check:

- `todo.md` identifies the unsupported call fragment without relying on
  testcase names.

### Step 2: Classify Frame-Slot Address GPR Call Argument Route

Goal: decide whether the selected family is a supportable lowering rule or a
narrower unsupported diagnostic.

Primary target:

- RV64 object-route same-module call lowering for GPR call arguments

Actions:

- Inspect the existing object-route call lowering path that rejects
  `FrameSlotAddress` GPR argument selections.
- Identify the smallest semantic boundary for materializing a local frame-slot
  address into the outgoing scalar pointer argument register.
- Check whether an existing helper already owns local frame address
  materialization for call arguments and can be reused without broadening sret
  support.
- Document any unsupported variants that must remain fail-closed.

Completion check:

- `todo.md` names the selected support or diagnostic route, the likely owned
  files/functions, and any adjacent fragments that remain out of scope.

### Step 3: Add Focused Backend Coverage

Goal: cover the selected frame-slot-address call-argument fragment from
prepared facts.

Primary target:

- Existing backend/object-route test area for RV64 call lowering diagnostics or
  semantic object-route lowering

Actions:

- Add focused coverage for a same-module scalar pointer call argument whose
  prepared source selection is `FrameSlotAddress`.
- Include adjacent fail-closed coverage if unsupported variants remain.
- Keep expectations tied to semantic call lowering or diagnostic behavior, not
  to testcase names.

Completion check:

- The focused test fails for the missing capability before implementation and
  proves the selected semantic boundary after implementation.

### Step 4: Implement Narrow Lowering Or Diagnostic Route

Goal: lower the selected supportable fragment or replace the generic diagnostic
with a narrower fail-closed route when support is not yet justified.

Primary target:

- RV64 object-route call argument lowering for the selected frame-slot-address
  GPR argument fragment

Actions:

- Implement the smallest semantic lowering rule justified by Step 2, or add a
  more precise unsupported diagnostic for the selected fragment.
- Avoid named-case branches, raw-string matching, or bypassing prepared object
  facts.
- Preserve existing parameter-home behavior, same-module sret behavior, and
  unrelated RV64 object-route diagnostics.

Completion check:

- Focused backend tests prove the semantic lowering or narrower diagnostic, and
  nearby unsupported fragments still fail closed.

### Step 5: Rerun Representative And Route Next Boundary

Goal: verify `va-arg-13.c` against the new boundary and record the next owner
if execution advances.

Primary target:

- `tests/c/external/gcc_torture/src/va-arg-13.c`

Actions:

- Rerun the representative with the supervisor-selected proof command.
- Document whether it now passes, reaches a later boundary, or remains blocked
  by a narrower unsupported diagnostic.
- Route any later boundary to an existing or new owner rather than expanding
  this idea silently.
- Escalate to broader validation if the implementation touches shared RV64
  object-route call lowering behavior.

Completion check:

- `todo.md` records the proof result for `va-arg-13.c` and identifies any
  later boundary or owner needed for follow-up lifecycle work.
