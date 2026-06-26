# RV64 Object Route Unsupported Instruction Fragment Runbook

Status: Active
Source Idea: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md
Activated from: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md

## Purpose

Turn the exposed RV64 object-route `unsupported_instruction_fragment` boundary
for `va-arg-13.c` and `920908-1.c` into a precise semantic lowering target or
a narrower fail-closed diagnostic.

## Goal

Identify the exact prepared/BIR instruction fragment behind the diagnostic,
classify its RV64 object-lowering requirement, and prove the smallest
supportable semantic route without testcase-shaped handling.

## Core Rule

Use prepared facts and BIR instruction semantics as the source of truth. Do not
infer behavior from testcase names, raw source syntax, or expectation rewrites.

## Read First

- `ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md`
- Existing backend object-route lowering diagnostics for
  `unsupported_instruction_fragment`
- Prepared/BIR dump tooling for
  `tests/c/external/gcc_torture/src/va-arg-13.c`
- Prepared/BIR dump tooling for
  `tests/c/external/gcc_torture/src/920908-1.c`

## Current Targets

- `tests/c/external/gcc_torture/src/va-arg-13.c`
- `tests/c/external/gcc_torture/src/920908-1.c`
- RV64 object-route BIR instruction lowering behind
  `unsupported_instruction_fragment`
- Focused backend tests for the selected instruction fragment

## Non-Goals

- Do not reopen scalar GPR stack-slot formal-home support from idea 374.
- Do not implement byval aggregate parameter homes, aggregate `va_arg` helper
  lowering, or frame-slot address call-argument materialization unless the
  captured fragment proves one is the correct existing owner.
- Do not reconstruct ABI placement, stack layout, or source intent from
  testcase names or raw syntax.
- Do not perform broad RV64 object-route rewrites unrelated to the observed
  unsupported instruction fragment.
- Do not downgrade expectations or convert supported-path coverage to
  unsupported coverage without explicit approval.

## Working Model

The parameter-home route has already advanced the known representatives to a
later generic object-route instruction-lowering boundary. This plan owns only
the next unsupported BIR instruction fragment. If inspection shows multiple
independent instruction families, split or route the extra families instead of
folding them into one oversized implementation step.

## Execution Rules

- Keep each packet tied to one plan step and update `todo.md` with proof.
- Prefer semantic lowering by BIR instruction kind, operand, type, storage, and
  prepared object-route facts.
- Preserve precise fail-closed diagnostics for fragments still outside this
  idea.
- Add focused backend coverage before treating either representative as proof.
- Rerun both known representatives after any lowering or diagnostic change.
- If a later boundary appears, document it in `todo.md` and route it to an
  existing or new owner instead of silently expanding this plan.

## Ordered Steps

### Step 1: Capture Unsupported Instruction Fragment Evidence

Goal: identify the exact prepared/BIR instruction fragment behind
`unsupported_instruction_fragment` for both known representatives.

Primary target:

- Prepared/BIR evidence for `va-arg-13.c`
- Prepared/BIR evidence for `920908-1.c`

Actions:

- Run the narrow diagnostic/dump commands needed to expose the object-route
  BIR instruction fragment for each representative.
- Record the relevant instruction kind, operands, object types, storage facts,
  and current fail-closed diagnostic path in `todo.md`.
- Compare the two representatives and note whether they share the same
  semantic fragment or expose separable families.

Completion check:

- `todo.md` contains enough prepared/BIR evidence for an executor to name the
  unsupported instruction fragment without relying on testcase names.

### Step 2: Classify The RV64 Object-Lowering Route

Goal: decide whether the captured fragment is one supportable lowering rule, a
narrower unsupported diagnostic, or multiple owner-worthy instruction families.

Primary target:

- RV64 object-route lowering code that emits
  `unsupported_instruction_fragment`

Actions:

- Inspect the existing object-route lowering path for the captured instruction
  kind and prepared object facts.
- Identify the smallest semantic support boundary that can be implemented and
  tested from prepared facts.
- If the captured evidence maps to an existing open owner, document that route
  and stop before duplicating scope.
- If the two representatives split into distinct families, ask the supervisor
  to route or create separate lifecycle work instead of broadening this plan.

Completion check:

- `todo.md` names the selected semantic route, the files/functions likely owned
  by the implementation packet, and any fragments that must remain fail-closed.

### Step 3: Add Focused Backend Coverage

Goal: create tests for the smallest semantic instruction fragment selected in
Step 2.

Primary target:

- Existing backend/object-route test area for RV64 lowering diagnostics or
  semantic object-route lowering

Actions:

- Add focused coverage that exercises the selected BIR instruction fragment
  from prepared facts.
- Include adjacent fail-closed coverage when unsupported variants remain.
- Keep expectations tied to semantic lowering or diagnostic behavior, not to
  testcase names.

Completion check:

- The focused test fails for the missing capability before the implementation
  packet and proves the intended semantic boundary after it.

### Step 4: Implement The Narrow Lowering Or Diagnostic Route

Goal: lower the selected supportable fragment or replace the generic diagnostic
with a narrower fail-closed route when support is not yet justified.

Primary target:

- RV64 object-route BIR instruction lowering for the selected fragment

Actions:

- Implement the smallest semantic lowering rule justified by Step 1 and Step 2,
  or add a more precise unsupported diagnostic for the selected fragment.
- Avoid named-case branches, raw-string matching, or bypassing prepared object
  facts.
- Preserve existing parameter-home behavior and unrelated RV64 object-route
  diagnostics.

Completion check:

- Focused backend tests prove the semantic lowering or narrower diagnostic, and
  nearby unsupported fragments still fail closed.

### Step 5: Rerun Known Representatives And Route Next Boundary

Goal: verify `va-arg-13.c` and `920908-1.c` against the new boundary and record
the next owner if execution advances.

Primary target:

- `tests/c/external/gcc_torture/src/va-arg-13.c`
- `tests/c/external/gcc_torture/src/920908-1.c`

Actions:

- Rerun both known representatives with the supervisor-selected proof command.
- Document whether each representative now passes, reaches a later boundary, or
  remains blocked by a narrower unsupported diagnostic.
- Route any later boundary to an existing or new owner rather than expanding
  this idea silently.
- Escalate to broader validation if the implementation touched shared RV64
  object-route lowering behavior.

Completion check:

- `todo.md` records the proof result for both representatives and identifies
  any later boundary or owner needed for follow-up lifecycle work.
