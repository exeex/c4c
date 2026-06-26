# RV64 Variadic `va_end` Object Boundary Runbook

Status: Active
Source Idea: ideas/open/388_rv64_object_route_variadic_va_end_boundary.md
Activated from: ideas/open/388_rv64_object_route_variadic_va_end_boundary.md

## Purpose

Resolve the next RV64 object-route boundary reached by
`tests/c/external/gcc_torture/src/va-arg-13.c` after frame-slot-address call
argument lowering: unresolved prepared calls to `llvm.va_end.p0`.

## Goal

Decide and implement the RV64 object-route behavior for prepared
`llvm.va_end.p0` call boundaries, then rerun `va-arg-13.c` and route any later
boundary.

## Core Rule

Use prepared/BIR variadic-helper facts as the source of truth. Do not hide the
unresolved symbol through linker tricks, fake extern definitions, testcase
names, or raw text offsets.

## Read First

- `ideas/open/388_rv64_object_route_variadic_va_end_boundary.md`
- `ideas/closed/386_rv64_object_route_unsupported_instruction_fragment.md`
- `build/agent_state/386_step3_va-arg-13.case.log`
- Prepared/BIR dumps for `tests/c/external/gcc_torture/src/va-arg-13.c`
- Existing RV64 object-route variadic helper lowering in
  `src/backend/mir/riscv/codegen/object_emission.cpp`

## Current Targets

- `tests/c/external/gcc_torture/src/va-arg-13.c`
- Prepared direct call boundary for `llvm.va_end.p0`
- RV64 object emission for variadic helper/intrinsic calls

## Non-Goals

- Do not reopen frame-slot-address GPR call-argument lowering from idea 386.
- Do not implement same-module memory-return/sret calls; idea 387 owns that
  family.
- Do not implement broad aggregate `va_arg` helper lowering unless it is proven
  necessary for the selected `va_end` boundary.
- Do not add fake symbols, linker allowlists, or unresolved-symbol masking.
- Do not downgrade expectations or weaken existing object-route contracts.

## Working Model

After idea 386, `va-arg-13.c` gets through c4c RV64 object compilation and
fails when the object is linked because calls to `llvm.va_end.p0` remain as
unresolved external references at `.text+0xac` and `.text+0xdc` in `test`.
The likely supportable semantic route is that `va_end` has no runtime work for
the prepared RV64 object path, but this must be proven from BIR/prepared facts
and existing variadic-helper conventions rather than assumed from the source
case.

## Execution Rules

- Keep changes scoped to the selected prepared `llvm.va_end.p0` boundary.
- Add focused backend/object-route coverage before treating `va-arg-13.c` as
  proof.
- Preserve fail-closed behavior for non-`va_end` intrinsic calls and unrelated
  variadic helpers.
- Rerun `va-arg-13.c` after any lowering or diagnostic change.
- If execution reaches a later boundary, record it in `todo.md` and route it to
  an existing or new idea instead of expanding this plan silently.
- Use backend proof for implementation packets; escalate if the selected route
  touches shared call lowering or broad variadic helper behavior.

## Ordered Steps

### Step 1: Capture The Prepared `va_end` Boundary

Goal: identify the exact prepared/BIR shape behind the unresolved
`llvm.va_end.p0` link failure.

Primary targets:

- `tests/c/external/gcc_torture/src/va-arg-13.c`
- Prepared dump focused on function `test`
- RV64 object case log at `build/agent_state/386_step3_va-arg-13.case.log`

Actions:

- Reproduce or inspect the existing link boundary for `llvm.va_end.p0`.
- Capture the semantic BIR and prepared call-plan facts for each `va_end` call.
- Check whether the call is represented as a prepared variadic helper, a direct
  extern fixed-arity call, or another object-route call shape.
- Identify the existing RV64 object-emission helper that should own this
  intrinsic, if one exists.

Completion check:

- `todo.md` records the exact `llvm.va_end.p0` call shape, instruction
  positions, prepared facts, and likely owned helper/function.

### Step 2: Classify Lowering Or Diagnostic Route

Goal: decide whether RV64 object emission should erase/lower `va_end` or emit a
narrower unsupported diagnostic.

Primary target:

- RV64 object-route variadic helper handling

Actions:

- Compare `va_end` with existing `va_start` and `va_arg` helper handling.
- Determine whether `va_end` is semantically a no-op for the prepared RV64
  object route.
- Define fail-closed variants that must not be accepted accidentally.
- Keep unrelated intrinsic and external-call behavior unchanged.

Completion check:

- `todo.md` names the selected route, exact guard facts, owned files/functions,
  and unsupported variants that remain out of scope.

### Step 3: Add Focused Backend Coverage

Goal: prove the selected `va_end` behavior with focused object-route tests.

Primary target:

- Existing RV64 backend object-emission test area

Actions:

- Add focused coverage for a prepared `llvm.va_end.p0` call boundary.
- If the selected behavior is no-op lowering, verify no relocation or external
  symbol remains for `llvm.va_end.p0`.
- Add fail-closed coverage for adjacent unsupported intrinsic or malformed
  prepared call shapes when practical.
- Keep expectations tied to prepared/BIR facts, not testcase text offsets.

Completion check:

- Focused tests fail before implementation and pass after the selected
  semantic route or narrowed diagnostic is implemented.

### Step 4: Implement Narrow RV64 Object Route

Goal: implement the selected `va_end` lowering or diagnostic without broad
variadic rewrites.

Primary target:

- RV64 object emission for prepared variadic helper calls

Actions:

- Implement the smallest route justified by Step 2.
- If lowering `va_end` as no-op, require exact prepared/BIR facts proving the
  intrinsic and argument shape.
- Preserve existing `va_start`, `va_arg`, ordinary extern calls, and same-module
  call behavior.
- Keep idea 387 sret handling out of scope.

Completion check:

- Focused backend tests prove the route and adjacent unsupported shapes still
  fail closed.

### Step 5: Rerun `va-arg-13.c` And Route Next Boundary

Goal: verify the representative against the new boundary and preserve the next
owner if execution advances.

Primary target:

- `tests/c/external/gcc_torture/src/va-arg-13.c`

Actions:

- Rerun the RV64 gcc-torture object runner for `va-arg-13.c`.
- Record whether the case now links/runs, reaches a later compile/link/runtime
  boundary, or fails with a narrower diagnostic.
- Route any later boundary to an existing or new owner rather than expanding
  this idea silently.
- Run the supervisor-selected backend proof before acceptance.

Completion check:

- `todo.md` records the representative result, proof command, proof log, and
  any later owner needed for follow-up lifecycle work.
