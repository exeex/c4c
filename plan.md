# BIR Scalar Control Flow Semantic Producer Admission

Status: Active
Source Idea: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Supersedes: combined scalar/signature/control runbook split after Step 1 producer-boundary inspection.

## Purpose

Repair BIR scalar-control-flow semantic producer admission without coupling it
to independent function-signature or scalar-binop producer boundaries.

## Goal

Publish or admit the missing BIR scalar-control-flow facts for the current
scalar-control-flow lane and prove representative RV64 rows advance because
the CFG/terminator/phi producer path is correct.

## Core Rule

Treat the outer `latest function failure` note as a diagnostic funnel only.
Implement through the real scalar-control-flow producer boundary, not through
function signatures, scalar binops, RV64 lowering, expectations, or allowlists.

## Read First

- `ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `src/backend/bir/lir_to_bir/module.cpp`
- `src/backend/bir/lir_to_bir/`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`

## Current Targets

- Scalar-control-flow semantic family rows, including representative
  `src/20000314-3.c` function `attr_eq`.
- Nearby scalar-control-flow rows recorded in the row inventory, including
  `src/20080502-1.c`, `src/930614-1.c`, `src/980604-1.c`,
  `src/ieee/fp-cmp-8*.c`, `src/ieee/pr38016.c`, `src/pr35456.c`, and
  `src/pr39501.c`.
- BIR CFG, terminator, and phi-lowering producer paths that create or reject
  scalar-control-flow semantic facts.

## Non-Goals

- Do not repair function-signature lowering in this runbook; it is now tracked
  by `ideas/open/561_bir_function_signature_semantic_producer_admission.md`.
- Do not repair scalar-binop instruction lowering in this runbook; it is now
  tracked by `ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.
- Do not use expectation rewrites, unsupported downgrades, allowlist edits, or
  named-case shortcuts as producer progress.
- Do not expand into RV64 ABI, object emission, local-memory, call metadata,
  runtime/intrinsic, or global/bootstrap work unless a new lifecycle route
  records a separate owner.

## Working Model

- Step 1 of the retired combined runbook proved that the shared outer BIR
  admission note is only a failure publication funnel.
- The scalar-control-flow producer boundary is in CFG/terminator/phi lowering,
  including `BirFunctionLowerer::lower()`, `collect_phi_lowering_plans()`,
  `lower_block_phi_insts()`, `initialize_aggregate_phi_state()`,
  `apply_pending_aggregate_phi_copies()`, and `lower_block_terminator()` in
  `src/backend/bir/lir_to_bir/module.cpp`.
- Capability progress requires semantic publication or fail-closed admission
  at this producer boundary before RV64 representative proof is accepted.

## Execution Rules

- Keep executor packet progress in `todo.md`; rewrite this runbook only for a
  real route correction or lifecycle transition.
- Add focused BIR coverage before or alongside producer repair.
- Prefer semantic publication and explicit fail-closed notes over downstream
  inference.
- Each code-changing step needs fresh build or compile proof plus the
  supervisor-selected narrow BIR/RV64 subset.
- Broaden validation before closure if CFG, terminator, or phi-lowering
  changes have wider backend impact.

## Steps

### Step 1: Add Scalar-Control-Flow BIR Coverage

Goal: Lock in expected BIR semantic fact publication or fail-closed admission
for scalar-control-flow rows.

Primary targets:
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- Existing BIR test helpers adjacent to semantic admission tests
- Representative scalar-control-flow inputs from the row inventory

Actions:
- Add or extend focused tests for the scalar-control-flow producer boundary.
- Cover the representative `src/20000314-3.c` function `attr_eq` or a stronger
  same-family substitute if the row inventory has changed.
- Keep tests producer-oriented; do not encode RV64 lowering assumptions as the
  first proof.

Completion check:
- Focused BIR coverage either fails before the producer repair or documents an
  already correct scalar-control-flow producer behavior that still needs RV64
  proof.

### Step 2: Repair Scalar-Control-Flow Producer Admission

Goal: Publish or admit the missing scalar-control-flow semantic facts through
the real CFG/terminator/phi producer boundary.

Primary targets:
- `src/backend/bir/lir_to_bir/module.cpp`
- Any narrow BIR semantic fact carrier or admission helper identified by Step 1

Actions:
- Implement the smallest semantic producer repair that covers the
  scalar-control-flow family.
- Keep fail-closed behavior for missing or ambiguous CFG, terminator, or phi
  facts.
- Avoid function-signature, scalar-binop, RV64 ABI, allowlist, and expectation
  changes.

Completion check:
- Focused BIR coverage from Step 1 passes.
- The original scalar-control-flow semantic admission failure is removed at
  the BIR layer, not hidden or reclassified downstream.

### Step 3: Prove Scalar-Control-Flow RV64 Representatives

Goal: Show scalar-control-flow RV64 representatives advance because BIR facts
are published correctly.

Primary targets:
- `src/20000314-3.c`
- Nearby same-family scalar-control-flow rows from the row inventory

Actions:
- Run the supervisor-selected narrow RV64 proof for scalar-control-flow
  representatives.
- Compare nearby same-family rows where practical so proof is not
  testcase-shaped.
- If a representative now fails later in RV64 lowering, record the downstream
  owner boundary instead of expanding this runbook.

Completion check:
- Representative rows either pass or advance past the original BIR semantic
  admission diagnostic with a clearly recorded downstream owner.

### Step 4: Broader Validation And Closure Decision

Goal: Decide whether the scalar-control-flow source idea is complete or needs
another focused runbook.

Primary targets:
- Focused BIR tests touched by this plan
- Supervisor-selected RV64 semantic admission subset
- Broader backend validation if CFG, terminator, or phi-lowering changed

Actions:
- Run the supervisor-selected acceptance proof.
- Confirm no expectations, unsupported markers, allowlists, or downstream
  shortcuts were used as producer progress.
- Record any remaining scalar-control-flow leftovers in `todo.md` for
  supervisor/plan-owner routing.

Completion check:
- Scalar-control-flow rows have BIR coverage and representative proof, or
  remaining work is explicitly routed to separate lifecycle state.
