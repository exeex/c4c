# RV64 Object Lowering For Prepared Control-Flow Fragments

Status: Active
Source Idea: ideas/open/563_rv64_object_lowering_control_flow_fragments.md
Activated from: downstream follow-up split after scalar-control-flow BIR producer admission advanced its representatives beyond the original BIR admission diagnostics.

## Purpose

Repair RV64 object lowering for prepared BIR control-flow fragments now exposed
by the retired scalar-control-flow producer runbook.

## Goal

Teach the RV64 object route to emit prepared terminator, move-bundle/select,
and `SelectInst` fragments for the affected scalar-control-flow
representatives without reopening BIR producer admission.

## Core Rule

Treat this as RV64 object-lowering work. Do not claim progress through row
classification, expectation rewrites, unsupported markers, allowlist edits, or
by routing the failures back to scalar-control-flow admission unless focused
BIR evidence proves the prepared facts are malformed.

## Read First

- `ideas/open/563_rv64_object_lowering_control_flow_fragments.md`
- `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`

## Current Targets

- Prepared terminator object fragments exposed by `src/20000314-3.c`,
  `src/930614-1.c`, and `src/pr35456.c`.
- Prepared move-bundle or select-publication object fragments exposed by
  `src/980604-1.c`.
- BIR `SelectInst` object fragments exposed by `src/pr39501.c`.
- RV64 object emission diagnostics currently shaped as
  `unsupported_terminator_fragment`, `unsupported_move_bundle_target_shape`,
  or `unsupported_instruction_fragment`.

## Non-Goals

- Do not repair BIR scalar-control-flow CFG, terminator, or phi producer
  admission in this runbook.
- Do not repair function-signature producer behavior, scalar-binop producer
  behavior, or scalar/local-memory semantic admission.
- Do not rewrite expectations, mark rows unsupported, edit allowlists, or add
  named torture-case shortcuts.
- Do not perform broad RV64 rewrites unrelated to prepared terminator/select
  object fragments.

## Working Model

- The source scalar-control-flow representatives already advanced beyond their
  original BIR `scalar-control-flow semantic family` diagnostics.
- The active owner boundary is RV64 object emission over already prepared BIR
  and prealloc facts.
- `fragment_for_prepared_terminator()` owns branch/conditional-branch object
  fragments and currently returns no fragment for unsupported prepared
  terminator shapes.
- Select publication and move-bundle admission flow through prepared edge
  publication helpers before object emission accepts or rejects the move
  bundle.
- Plain BIR instruction lowering flows through
  `fragment_for_prepared_instruction()` and reports unsupported instruction
  fragments when `SelectInst` or its carrier shape is not object-emittable.

## Execution Rules

- Keep executor packet progress in `todo.md`; rewrite this runbook only for a
  true route correction or lifecycle transition.
- Add focused backend/RV64 object coverage before or alongside each repair.
- Preserve fail-closed diagnostics for malformed prepared facts.
- Prefer semantic RV64 lowering rules for fragment shapes over named-case
  matching.
- Each code-changing step needs fresh build proof plus the
  supervisor-selected narrow backend/RV64 subset.
- Before closure, run a broader backend check if object emission or prepared
  publication changes affect shared paths.

## Steps

### Step 1: Lock Down Current RV64 Object Fragment Failures

Goal: Create focused proof for the prepared fragment shapes that currently
block the scalar-control-flow representatives.

Primary targets:
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`
- Current per-row RV64 logs under the supervisor-selected proof subset

Actions:
- Add or extend focused tests for the prepared terminator, select-publication
  move-bundle, and `SelectInst` object-fragment diagnostics.
- Tie each test to the fragment shape, not to a single torture filename.
- Record which representative rows map to which focused object-lowering
  blocker in `todo.md` during execution.

Completion check:
- Focused coverage fails before the repair or documents an already precise
  fail-closed diagnostic for each target fragment family.

### Step 2: Repair Prepared Terminator Object Lowering

Goal: Emit RV64 object fragments for prepared terminator shapes exposed by the
control-flow representatives.

Primary targets:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `fragment_for_prepared_terminator()`
- Prepared branch-condition and compare-publication helpers used by
  terminator emission

Actions:
- Inspect the prepared facts for `src/20000314-3.c`, `src/930614-1.c`, and
  `src/pr35456.c` to identify the missing terminator lowering rule.
- Implement the narrow RV64 emission rule for the semantic terminator fragment
  shape.
- Keep malformed or unsupported prepared facts fail-closed with a diagnostic.

Completion check:
- Focused terminator coverage passes.
- The terminator representatives advance beyond
  `unsupported_terminator_fragment` because the RV64 terminator fragment is
  emitted.

### Step 3: Repair Prepared Move-Bundle And Select Publication Lowering

Goal: Emit or admit RV64 object moves for prepared select-publication bundles
without weakening publication contracts.

Primary targets:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:
- Identify the prepared move-bundle target shape exposed by `src/980604-1.c`.
- Repair the RV64 object move path or admission predicate for that semantic
  shape.
- Preserve rejection for ambiguous, multi-owner, or unsupported move bundles.

Completion check:
- Focused move-bundle/select-publication coverage passes.
- `src/980604-1.c` advances beyond `unsupported_move_bundle_target_shape`
  because the prepared publication can be emitted or correctly admitted.

### Step 4: Repair BIR SelectInst Object Lowering

Goal: Emit RV64 object fragments for the prepared `SelectInst` shape exposed by
the current representative.

Primary targets:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `fragment_for_prepared_instruction()`
- `fragment_for_prepared_select()`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:
- Inspect the `src/pr39501.c` `float_min1` `SelectInst` shape and any nearby
  focused backend fixture.
- Implement the smallest RV64 lowering or prepared-consumer admission repair
  for that semantic select fragment.
- Keep select-carrier aliases and join-transfer carriers distinct from plain
  scalar select emission.

Completion check:
- Focused `SelectInst` object coverage passes.
- `src/pr39501.c` advances beyond the `SelectInst`
  `unsupported_instruction_fragment` blocker.

### Step 5: Prove Representatives And Route Remaining Failures

Goal: Verify the repaired object-lowering capabilities on the affected RV64
representatives and split any new owner boundaries.

Primary targets:
- `src/20000314-3.c`
- `src/930614-1.c`
- `src/pr35456.c`
- `src/980604-1.c`
- `src/pr39501.c`

Actions:
- Run the supervisor-selected RV64 proof subset for the target
  representatives.
- Compare nearby same-fragment rows where practical so proof is not
  testcase-shaped.
- Record any remaining downstream owner boundary in `todo.md`; create or
  request a separate idea only if it is outside this source idea.

Completion check:
- The targeted representatives pass or advance beyond their current RV64
  object-lowering diagnostics because the prepared object fragments are
  correctly lowered.

### Step 6: Broader Validation And Closure Decision

Goal: Decide whether the RV64 object-lowering source idea is complete or needs
another focused runbook.

Primary targets:
- Focused backend tests touched by this plan
- Supervisor-selected RV64 object/progress subset
- Broader backend validation if shared object emission paths changed

Actions:
- Run the supervisor-selected acceptance proof.
- Confirm no expectation, unsupported marker, allowlist, or named-case shortcut
  was used as progress.
- Leave close-time regression guard to the plan-owner close flow when the
  supervisor delegates closure.

Completion check:
- Focused coverage and representative proof satisfy the source idea, or
  remaining work is explicitly routed to separate lifecycle state.
