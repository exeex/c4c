# BIR Scalar Local-Memory Semantic Admission Follow-Up

Status: Active
Source Idea: ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md
Activated from: downstream follow-up after scalar-control-flow representatives advanced beyond their original BIR admission diagnostics and the RV64 object-lowering follow-up closed.

## Purpose

Repair the scalar/local-memory semantic admission failures now exposed by IEEE
representatives after the scalar-control-flow and RV64 object-lowering lanes
advanced their own owner boundaries.

## Goal

Identify and repair the BIR scalar/local-memory producer or lowering rule that
causes the current `scalar/local-memory semantic family` diagnostics, then
prove the affected IEEE representatives advance for semantic reasons.

## Core Rule

Treat this as BIR scalar/local-memory semantic producer work. Do not claim
progress through row classification, expectation rewrites, unsupported markers,
allowlist edits, named IEEE-row shortcuts, or by routing the failures back to
scalar-control-flow without focused evidence of a real regression there.

## Read First

- `ideas/open/564_bir_scalar_local_memory_semantic_admission_followup.md`
- `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
- `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/scalar.cpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- Existing focused BIR semantic admission tests under `tests/backend/bir/`
- Relevant backend progress tests for the IEEE representatives

## Current Targets

- `src/ieee/fp-cmp-8.c`
- `src/ieee/fp-cmp-8f.c`
- `src/ieee/fp-cmp-8l.c`
- `src/ieee/pr38016.c`
- Current diagnostics shaped as `scalar/local-memory semantic family`

## Non-Goals

- Do not repair scalar-control-flow CFG, terminator, or phi producer behavior.
- Do not repair function-signature producer behavior.
- Do not repair scalar-binop producer behavior unless the diagnostics prove
  the active blocker is actually the scalar-binop owner boundary.
- Do not reopen RV64 object-lowering fragments from the closed object-lowering
  follow-up.
- Do not rewrite expectations, mark rows unsupported, edit allowlists, or add
  named IEEE testcase shortcuts.

## Working Model

- The scalar-control-flow representatives already advanced beyond their
  original BIR scalar-control-flow admission diagnostics.
- The RV64 object-lowering follow-up has closed; current IEEE-row blockers are
  expected to belong to scalar/local-memory semantic admission unless fresh
  evidence says otherwise.
- The likely producer area is the scalar/local-memory instruction-lowering
  path through `lower_scalar_or_local_memory_inst()` in
  `src/backend/bir/lir_to_bir/memory/coordinator.cpp` and helper routines in
  `src/backend/bir/lir_to_bir/scalar.cpp`.
- Remaining failures outside scalar/local-memory should be routed to their own
  owner boundary instead of expanding this runbook.

## Execution Rules

- Keep executor packet progress in `todo.md`; rewrite this runbook only for a
  true route correction or lifecycle transition.
- Start with focused diagnostics and producer-boundary evidence before changing
  lowering behavior.
- Add focused BIR/backend coverage before or alongside each repair.
- Preserve fail-closed diagnostics when semantic facts are absent, malformed,
  or owned by another boundary.
- Prefer semantic scalar/local-memory lowering rules over operand-shape or
  filename-specific matching.
- Each code-changing step needs fresh build proof plus the
  supervisor-selected narrow BIR/backend subset.
- Before closure, run a broader backend or semantic-admission validation subset
  if shared scalar/local-memory lowering changed.

## Steps

### Step 1: Lock Down Current Scalar/Local-Memory Diagnostics

Goal: Identify the exact current scalar/local-memory admission blockers and the
producer path responsible for them.

Primary targets:
- `src/ieee/fp-cmp-8.c`
- `src/ieee/fp-cmp-8f.c`
- `src/ieee/fp-cmp-8l.c`
- `src/ieee/pr38016.c`
- Diagnostic output from the supervisor-selected proof subset

Actions:
- Reproduce or inspect the current diagnostics for the affected IEEE rows.
- Map each blocker to the BIR scalar/local-memory producer path, including the
  relevant instruction kind, operand shape, and missing or malformed semantic
  fact.
- Compare against scalar-control-flow and scalar-binop boundaries so this
  runbook does not absorb another owner family.
- Record the row-to-boundary mapping in `todo.md`.

Completion check:
- The active blocker is assigned to a concrete scalar/local-memory producer or
  fail-closed admission boundary, or `todo.md` records why the route must be
  reviewed before implementation.

### Step 2: Add Focused Semantic Admission Coverage

Goal: Make the scalar/local-memory producer contract observable before or
alongside the repair.

Primary targets:
- Focused BIR semantic admission tests under `tests/backend/bir/`
- Existing backend progress tests for the IEEE rows when useful

Actions:
- Add or extend focused tests that capture the missing scalar/local-memory
  semantic facts or the required fail-closed rejection.
- Tie coverage to the semantic producer behavior rather than a single IEEE
  filename.
- Keep any backend representative assertions focused on advancement beyond the
  current scalar/local-memory diagnostic.

Completion check:
- Focused coverage demonstrates the current missing fact, malformed fact, or
  fail-closed admission behavior for the scalar/local-memory boundary.

### Step 3: Repair Scalar/Local-Memory Producer Behavior

Goal: Publish or reject the required semantic facts at the real
scalar/local-memory producer boundary.

Primary targets:
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`
- `src/backend/bir/lir_to_bir/scalar.cpp`
- Any narrow helper that owns the identified scalar/local-memory fact

Actions:
- Implement the smallest semantic producer or lowering repair for the
  identified scalar/local-memory shape.
- Preserve distinct handling for scalar-binop, scalar-control-flow, and
  function-signature owner boundaries.
- Keep malformed or unsupported inputs fail-closed with a precise diagnostic.

Completion check:
- Focused scalar/local-memory coverage passes.
- The repaired path publishes the required semantic fact or rejects the
  unsupported shape at the correct producer boundary.

### Step 4: Prove IEEE Representatives And Route Downstream Failures

Goal: Verify the affected IEEE rows advance because scalar/local-memory facts
are correct, then classify any remaining failures.

Primary targets:
- `src/ieee/fp-cmp-8.c`
- `src/ieee/fp-cmp-8f.c`
- `src/ieee/fp-cmp-8l.c`
- `src/ieee/pr38016.c`

Actions:
- Run the supervisor-selected proof subset for the target representatives.
- Compare nearby same-family rows where practical so proof is not
  testcase-shaped.
- Record any remaining downstream owner boundary in `todo.md`; create or
  request a separate idea only if it is outside this source idea.

Completion check:
- The affected representatives pass or advance beyond the
  `scalar/local-memory semantic family` diagnostic because semantic facts are
  produced or rejected fail-closed correctly.

### Step 5: Broader Validation And Closure Decision

Goal: Decide whether the scalar/local-memory source idea is complete or needs a
follow-up runbook.

Primary targets:
- Focused tests touched by this plan
- Supervisor-selected semantic-admission and backend progress subset
- Broader backend validation if shared scalar/local-memory paths changed

Actions:
- Run the supervisor-selected acceptance proof.
- Confirm no expectation, unsupported marker, allowlist, or named-case shortcut
  was used as progress.
- Leave close-time regression guard to the plan-owner close flow when the
  source idea is ready to close.

Completion check:
- The source idea either satisfies its acceptance criteria and is ready for
  plan-owner closure, or `todo.md` records the remaining owner boundary and
  next lifecycle decision.
