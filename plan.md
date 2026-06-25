# RV64 Object Route Pointer-Value Local Memory Runbook

Status: Active
Source Idea: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md

## Purpose

Repair the RV64 object route for the local-memory forms that still block idea
368 representatives after the focused frame-slot subobject-offset admission.

## Goal

Admit semantic RV64 object lowering for prepared pointer-value local-memory
loads and stores where prepared metadata proves the access width, alignment,
offset, and value/register facts are supportable.

## Core Rule

Use prepared addressing and value-home metadata as the source of truth. Do not
hard-code representative names, source offsets, frame-slot ids, or source-level
layout assumptions.

## Read First

- `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`
- `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`
- `build/agent_state/368_step1_20000217-1.memory-audit.txt`
- `build/agent_state/368_step1_20000121-1.memory-audit.txt`
- `build/agent_state/368_step1_va-arg-13.memory-audit.txt`
- `build/agent_state/368_step3_representatives.log`
- current RV64 object-emission local-memory handling in
  `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused backend tests in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Scope

- Preserve the existing focused support for direct frame-slot subobject
  local-memory loads and stores.
- Audit the failing representative instruction paths before each code packet,
  because the shared diagnostic can mask multiple owners.
- Implement pointer-value local-memory loads and stores only when the pointer
  source is already available in a GPR, the immediate offset is encodable, the
  access width/alignment is supported, and prepared metadata says the address
  shape is compatible.
- Keep unsupported pointer bases, widths, alignments, address spaces,
  volatility, aggregates, and dynamic layouts fail-closed with precise
  diagnostics.
- Rerun the three representatives and record whether each case passes,
  advances to the call-argument materialization owner, or exposes a different
  child idea.

## Non-Goals

- Do not implement frame-slot address call-argument materialization here; that
  belongs to `ideas/open/372_rv64_object_route_frame_slot_address_call_args.md`.
- Do not implement aggregate `va_arg` helper lowering.
- Do not implement byval aggregate parameter homes.
- Do not implement general terminator lowering or control-flow rewrites.
- Do not infer memory layout from source syntax.
- Do not skip, allowlist-filter, or downgrade gcc_torture runtime checks.

## Working Model

The previous Step 2 slice was useful focused capability for final
`slot.offset_bytes + access.address.byte_offset` composition, but Step 3 proved
it was not representative progress for the current residual bucket. The three
representatives now need two different owners:

- pointer-value local memory:
  - `src/20000217-1.c`: `showbug` has i16 pointer-value loads/stores from
    `%p.a` and `%p.b`.
  - `src/20000121-1.c`: `doit` has an i8 pointer-value load from `%p.id`.
  - `src/va-arg-13.c`: `dummy` has pointer-value loads/stores from `%p.vap`
    and a dependent i32 load from `%t0`.
- frame-slot address call arguments:
  - `src/20000217-1.c`: `main` passes `%lv.x` and `%lv.y` addresses using
    `local_frame_address_materialization`.
  - `src/va-arg-13.c`: `test` passes frame-slot addresses for `%t7` and
    `%t14` and records missing frame-slot arg publications.

This plan owns the pointer-value local-memory route first. If a representative
then advances to the frame-slot address call-argument boundary, route that
evidence to idea 372 instead of expanding this runbook.

## Execution Rules

- Start each implementation packet by confirming the concrete rejected access
  in the prepared dump or case-specific analysis log.
- Keep the first implementation packet narrow: support one semantic
  pointer-value load/store family and prove fail-closed neighbors.
- Add focused object-emission tests before relying on gcc_torture
  representatives.
- Use `test_after.log` only for the delegated proof command selected by the
  supervisor; put analysis logs under `build/agent_state/`.
- Treat any representative movement to aggregate helper, byval, terminator, or
  frame-slot address call-argument work as a route handoff, not silent scope
  expansion.

## Steps

### Step 1: Reclassify Representative Blockers

Goal: Replace the old subobject-offset route with the actual residual blocker
families.

Actions:

- Record that the focused subobject-offset frame-slot slice is retained as
  useful capability but did not satisfy representative movement.
- Confirm the three representatives still fail at
  `unsupported_local_memory_access`.
- Assign pointer-value local memory to this plan and frame-slot address
  call-argument materialization to idea 372.

Completion Check:

- `todo.md` points at the first pointer-value local-memory implementation
  packet and names idea 372 as the separate call-argument owner.

### Step 2: Admit Pointer-Value Scalar Accesses

Goal: Lower the first supportable RV64 pointer-value local-memory loads and
stores.

Actions:

- Support prepared pointer-value local-memory accesses when the pointer source
  is in a GPR and the access is scalar, nonvolatile, default address space,
  encodable, aligned, and in a supported integer or pointer width.
- Use prepared access offset and width metadata to select RV64 load/store
  opcodes.
- Keep missing pointer registers, unsupported widths, bad alignment,
  non-default address spaces, volatile accesses, aggregate payloads, and dynamic
  layout uncertainty rejected with precise diagnostics.
- Add focused object-emission tests for admitted pointer-value loads/stores and
  fail-closed neighbors.

Completion Check:

- Focused backend object-emission coverage proves the admitted pointer-value
  form and fail-closed diagnostics.

### Step 3: Rerun Representatives and Route Residuals

Goal: Prove whether pointer-value local-memory support advances the residual
bucket.

Actions:

- Rerun `src/20000217-1.c`, `src/20000121-1.c`, and `src/va-arg-13.c` through
  the RV64 gcc_torture backend runner.
- Record which representatives pass, still fail on in-scope pointer-value
  local memory, or advance to idea 372 / aggregate `va_arg` / byval /
  terminator work.
- If no representative moves, stop for route review instead of adding another
  narrow focused-only slice.

Completion Check:

- `todo.md` records the representative command, pass/fail count, and remaining
  owner for every listed case.

### Step 4: Closure or Handoff Decision

Goal: Decide whether idea 368 satisfies its acceptance criteria or should stay
open for another in-scope memory packet.

Actions:

- Compare focused tests and representative evidence against the source idea's
  acceptance criteria.
- Confirm at least one representative advanced due to semantic local-memory
  repair before claiming progress for idea 368.
- Route frame-slot address call-argument evidence to idea 372 and other
  residual owners to their existing child ideas.
- Ask the plan-owner close flow to archive idea 368 only if the source idea is
  actually complete and regression guard passes.

Completion Check:

- The supervisor has a clear close/defer/handoff decision for idea 368 with
  proof paths recorded in `todo.md`.
