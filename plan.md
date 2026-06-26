# RV64 Same-Module Sret Call Runbook

Status: Active
Source Idea: ideas/open/387_rv64_object_route_same_module_sret_calls.md
Activated from: ideas/open/387_rv64_object_route_same_module_sret_calls.md

## Purpose

Repair or precisely route the RV64 object-route same-module call family whose
prepared call plan carries ABI memory-return/sret state. The representative is
`tests/c/external/gcc_torture/src/920908-1.c`, which currently reaches an
admission gate before ordinary argument lowering.

## Goal

Identify the exact same-module sret call fragment from prepared/BIR and
call-plan evidence, then add the narrow supported lowering or a precise
fail-closed diagnostic with focused backend proof.

## Core Rule

Use prepared call-plan and object facts for sret ownership. Do not delete the
`memory_return` gate generically, and do not hard-code `920908-1.c`, callee
`f`, stack offsets, or the current diagnostic as the route condition.

## Read First

- `ideas/open/387_rv64_object_route_same_module_sret_calls.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- backend prepared/call-plan surfaces that describe `memory_return`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/c/external/gcc_torture/src/920908-1.c`

## Current Targets

- Representative: `tests/c/external/gcc_torture/src/920908-1.c`
- RV64 object-route same-module `bir::CallInst` with
  `call_plan->memory_return`
- sret address placement into the call ABI
- post-call memory result/object state
- focused backend object-emission coverage for the smallest supportable sret
  call fragment or a narrow unsupported diagnostic

## Non-Goals

- Do not reopen ordinary frame-slot-address scalar GPR call arguments.
- Do not reopen scalar GPR stack-slot formal-home support from idea 374.
- Do not fold byval aggregate parameter homes, aggregate `va_arg`, or unrelated
  call families into this owner.
- Do not perform a broad RV64 call-lowering rewrite.
- Do not downgrade expectations, filter allowlists, or rename diagnostics as
  progress.

## Working Model

Idea 386 split this representative out because its same-module call carries
memory-return/sret state:

```text
bir.call void f(ptr sret(size=4, align=4) %t8, i32 2, i64 %t4, i64 %t7)
```

The return object home is a frame slot, and the sret address is represented
through local frame address materialization. The current object route rejects
the call at the `call_plan->memory_return.has_value()` admission gate before it
classifies the ordinary arguments. This plan should first prove the exact
prepared facts, then decide whether RV64 object emission can support the
fragment safely or must fail closed under a narrower owner.

## Execution Rules

- Refresh evidence before implementing; do not rely only on the stale split
  note.
- Keep sret address placement, normal scalar arguments, and post-call memory
  result behavior separate in notes and tests.
- Add focused backend coverage before or alongside implementation.
- Preserve existing scalar call-argument support and diagnostics.
- If the representative advances to a later boundary, record that boundary in
  `todo.md` for lifecycle routing instead of expanding this plan silently.

## Steps

### Step 1: Refresh Same-Module Sret Evidence

Goal: capture the exact prepared/BIR/call-plan shape for the representative.

Primary target: `tests/c/external/gcc_torture/src/920908-1.c`.

Actions:

- Generate or inspect current prepared/BIR and call-plan evidence for the
  representative.
- Identify the memory-return object, sret address source, ordinary arguments,
  placements, and current rejection point.
- Confirm whether the failure is still the `memory_return` admission gate.
- Record the exact evidence and owner classification in `todo.md`.

Completion check: `todo.md` names the current same-module sret fragment,
prepared facts, call-plan state, and whether the old admission-gate boundary
still holds.

### Step 2: Choose The Narrow Sret Route

Goal: decide the minimal semantic route for the selected same-module sret call
family.

Primary target: RV64 object emission and prepared call-plan metadata.

Actions:

- Decide whether the supported route should materialize the sret address as an
  ABI argument, publish post-call memory state, or fail closed with a narrower
  diagnostic.
- Define required facts and ambiguous/unsupported variants.
- Keep ordinary scalar frame-slot-address call arguments outside this owner
  unless the same abstraction is directly required and idea 386 remains
  aligned.
- Record the route, guards, and proof expectations in `todo.md`.

Completion check: `todo.md` gives an executor a concrete route and guard set
without re-deriving lifecycle scope.

### Step 3: Add Focused Backend Coverage

Goal: pin the selected same-module sret behavior before or alongside code
changes.

Primary target: focused RV64 backend object-emission tests.

Actions:

- Add a fixture for a same-module call with memory-return/sret state matching
  the representative fragment.
- Assert correct sret address placement and ordinary argument preservation for
  supported behavior, or assert the precise fail-closed diagnostic for
  unsupported behavior.
- Add adjacent malformed/ambiguous coverage where the selected route requires
  fail-closed protection.
- Avoid testcase-shaped assertions tied to `920908-1.c` or callee `f`.

Completion check: focused coverage fails on the current missing/narrow route
and expresses the intended support or diagnostic contract.

### Step 4: Implement Or Narrowly Route Sret Calls

Goal: implement the selected route or precise unsupported diagnostic without a
broad call-lowering rewrite.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp` and direct
helpers if required.

Actions:

- Use prepared call-plan/object facts for sret address placement and
  memory-return state.
- Preserve ordinary argument lowering behavior.
- Keep ambiguous or unsupported memory-return shapes fail-closed.
- Run the delegated backend proof command and record results in `todo.md`.

Completion check: focused backend coverage passes, existing relevant backend
coverage remains green, and `todo.md` records the exact proof command.

### Step 5: Rerun `920908-1.c` And Route The Next Boundary

Goal: verify the representative advances or document the next owner.

Primary target: `tests/c/external/gcc_torture/src/920908-1.c`.

Actions:

- Rerun the representative with the RV64 gcc_torture backend object case
  runner.
- Confirm whether the prior same-module sret boundary is gone.
- If a later boundary appears, record exact evidence and route it to an
  existing or new idea instead of expanding this plan.
- Preserve the backend proof result required by the supervisor.

Completion check: `todo.md` records representative result, proof logs, and
either completion evidence for idea 387 or a clearly owned later boundary.
