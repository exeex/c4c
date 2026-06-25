# RV64 Object Route Frame-Slot Base-Offset Local Memory Runbook

Status: Active
Source Idea: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md

## Purpose

Repair the RV64 object route for prepared local-memory accesses that need a
frame-slot base plus subobject or nonzero offset, starting from the residual
representatives split out of idea 354.

## Goal

Admit semantic RV64 object lowering for supportable prepared frame-slot
base-plus-offset local memory forms while keeping unsupported width, alignment,
offset, and aggregate shapes fail-closed with precise diagnostics.

## Core Rule

Use prepared frame-slot and memory metadata as the source of truth. Do not infer
case-specific layout from gcc_torture source text, hard-code representative
offsets, or weaken runtime expectations.

## Read First

- `ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md`
- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `build/agent_state/354_step3_representative_refresh.log`, if present
- prepared-BIR dumps for:
  - `src/20000217-1.c`
  - `src/20000121-1.c`
  - `src/va-arg-13.c`
- current RV64 object-emission local-memory handling in
  `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused backend tests in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Scope

- Audit prepared local-memory facts and frame-slot metadata for the three
  representatives.
- Define the first supportable RV64 address forms for frame-slot
  base-plus-offset loads and stores.
- Implement object-emission support for those forms through prepared metadata.
- Preserve precise unsupported diagnostics for shapes that remain outside the
  idea.
- Add focused backend tests for both admitted and rejected local-memory forms.
- Rerun all listed representatives and record their outcomes.

## Non-Goals

- Do not implement aggregate `va_arg` helper lowering.
- Do not implement byval aggregate parameter homes.
- Do not implement general terminator lowering or control-flow rewrites.
- Do not broaden into source-syntax layout inference.
- Do not skip, allowlist-filter, or downgrade gcc_torture runtime checks.

## Working Model

Idea 354 has already classified this residual bucket as a concrete RV64 object
route memory-addressing gap, not an opaque prepared-module-shape failure.
Prepared lowering is expected to expose enough slot identity, offset, width,
and value facts for target emission to select RV64 stack-relative load/store
forms where the offset is encodable and the width is supported.

## Execution Rules

- Start from diagnostics and prepared dumps before editing target emission.
- Keep the first implementation slice narrow: one supportable width and offset
  family is acceptable if unsupported cases stay explicit.
- Add focused tests before relying on gcc_torture representatives.
- Use `test_after.log` only for the delegated proof command selected by the
  supervisor; put analysis logs under `build/agent_state/`.
- Escalate to a separate idea if representative progress requires aggregate
  helper lowering, byval homes, or terminator work instead of local-memory
  addressing.

## Steps

### Step 1: Audit Prepared Frame-Slot Memory Facts

Goal: Identify the exact prepared local-memory shape that triggers
`unsupported_local_memory_access` in the representatives.

Actions:

- Generate or inspect prepared dumps for `src/20000217-1.c`,
  `src/20000121-1.c`, and `src/va-arg-13.c`.
- Locate the object-emission rejection path and record which metadata fields
  are available for slot base, offset, width, alignment, signedness, and access
  direction.
- Separate cases that are pure frame-slot base-plus-offset memory accesses from
  cases blocked first by aggregate helper or other nonlocal-memory work.

Completion Check:

- `todo.md` records the supportable first address form, remaining rejected
  forms, and the focused proof target for implementation.

### Step 2: Admit the First Semantic Address Form

Goal: Lower one or more supportable prepared frame-slot base-plus-offset
load/store forms in the RV64 object route.

Actions:

- Implement the object-emission path using prepared frame-slot and memory
  metadata.
- Check RV64 immediate range and alignment before emitting instructions.
- Keep unsupported widths, offsets, alignments, aggregates, or missing metadata
  rejected with precise diagnostics.
- Add focused object-emission tests for admitted and fail-closed cases.

Completion Check:

- Focused backend object-emission coverage proves the admitted forms and
  fail-closed diagnostics.

### Step 3: Rerun Representatives

Goal: Prove the local-memory repair advances the residual bucket without
overfitting a named case.

Actions:

- Rerun `src/20000217-1.c`, `src/20000121-1.c`, and `src/va-arg-13.c` through
  the RV64 gcc_torture backend runner.
- Record which representatives pass, advance to a different owner, or still
  fail on in-scope local-memory addressing.
- If a representative now blocks on aggregate helper, byval, or terminator
  work, route that evidence to the existing child idea instead of expanding
  this plan.

Completion Check:

- `todo.md` records the representative command, pass/fail count, and remaining
  owner for every listed case.

### Step 4: Closure Decision

Goal: Decide whether idea 368 satisfies its acceptance criteria.

Actions:

- Compare focused tests and representative evidence against the source idea's
  acceptance criteria.
- Confirm all remaining local-memory forms either pass, have precise
  diagnostics, or are documented as out of scope.
- Ask the plan-owner close flow to archive idea 368 only if the source idea is
  actually complete and regression guard passes.

Completion Check:

- The supervisor has a clear close/defer decision for idea 368 with proof
  paths recorded in `todo.md`.
