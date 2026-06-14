# Phase F3 x86 Compare-Join LoadLocal Selected-Arm Support

Status: Active
Source Idea: ideas/open/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md

## Purpose

Add the bounded x86 production support that idea 261 needs before it can create
a supported proof surface for idea 258.

## Goal

Teach the prepared compare-join return path to classify and render one selected
arm rooted in a same-predecessor-block `LoadLocal` through the existing Route 3
statement-memory agreement facade.

## Core Rule

Keep the route production-real and narrow. Do not prove this by injecting
synthetic publications, weakening compare-join expectations, or broadening into
generic selected-value lowering.

## Read First

- `ideas/open/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md`
- `ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md`
- `ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md`
- `todo.md` Step 2 blocker history from idea 261.
- `src/backend/mir/x86/module/module.cpp`
- prepared materialized compare-join classification and render-contract helpers.
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`

## Current Targets

- Materialized compare-join return classification for selected
  same-predecessor-block `LoadLocal`.
- x86 rendering through
  `render_agreed_route3_load_local_statement_memory_operand(...)` or an
  equivalent narrow helper that keeps the same authority checks.
- Focused x86 handoff coverage for positive selected-`LoadLocal` support and
  fail-closed prepared metadata rejection.
- Existing compare-join supported shapes and output spelling.

## Non-Goals

- Do not implement generic compare-join selected-expression lowering.
- Do not rewrite x86 addressing, register materialization, frame layout, or
  emitted output policy beyond the selected `LoadLocal` arm.
- Do not make prepared `memory_accesses` the authority for Route 3 selected
  memory facts.
- Do not change RISC-V behavior.
- Do not weaken existing route-debug, fallback, prepared status, or output
  contracts.

## Working Model

- Idea 258 owns the Route 3/prepared agreement facade.
- Idea 261 owns the supported fixture/proof surface after the compare-join
  route can reach that facade.
- This runbook owns the missing production bridge in the x86 materialized
  compare-join return path.
- Existing immediate, param, same-module global, pointer-backed global, and
  trailing-immediate shapes are compatibility constraints.

## Execution Rules

- Start from the idea 261 Step 2 blocker and reproduce only enough of the
  fixture shape to understand the production route.
- Prefer adding a narrow classified shape and render branch over generalized
  expression support.
- Use existing Route 3 and prepared publication lookup helpers instead of
  comparing unrelated id domains directly.
- Keep tests focused on production compare-join behavior; avoid fixture-only
  shortcuts and expectation weakening.
- Use x86-enabled validation for x86-only CTest targets.

## Steps

### Step 1: Contract Inventory

Goal: identify the exact prepared and x86 contract points that reject the
selected `LoadLocal` arm.

Actions:
- Inspect the materialized compare-join classification and render-contract
  helpers used by
  `append_prepared_i32_param_zero_compare_join_return_function(...)`.
- Map where the selected arm currently becomes immediate, param, global, or
  pointer-backed global, and where `LoadLocal` is rejected.
- Identify the existing Route 3 and prepared publication facts needed by
  `render_agreed_route3_load_local_statement_memory_operand(...)`.
- Record the minimal extension target in `todo.md`.

Completion check:
- `todo.md` names the precise classification helper, render-contract helper,
  x86 render site, and authority facts for the selected `LoadLocal` arm.

### Step 2: Selected LoadLocal Classification

Goal: add the minimal prepared compare-join contract support for a selected
same-predecessor-block `LoadLocal` arm.

Actions:
- Extend only the selected-arm classification/render-contract data needed for
  a same-predecessor-block `LoadLocal` with prepared local-slot addressing.
- Preserve all existing supported compare-join shapes and failure statuses.
- Add focused tests for classification ownership and rejected drift where the
  prepared contract cannot authoritatively identify the selected `LoadLocal`.

Completion check:
- The prepared compare-join contract can represent the selected `LoadLocal`
  arm without accepting unrelated selected-value shapes.

### Step 3: x86 Render Bridge

Goal: render the selected `LoadLocal` arm through the existing Route 3
statement-memory agreement facade.

Actions:
- Add the narrow x86 render branch for the new selected-arm contract.
- Reuse the existing agreement facade or factor a small helper that preserves
  the same Route 3/prepared source-memory checks.
- Fail closed on missing, incomplete, mismatched, or unsupported authoritative
  prepared metadata instead of falling back past the compare-join handoff.
- Preserve current x86 output for existing compare-join shapes.

Completion check:
- The joined-branch selected-`LoadLocal` route reaches the agreement facade and
  existing compare-join fixtures remain stable.

### Step 4: Focused Proof and Resume Decision

Goal: prove the bounded production support and decide whether idea 261 can
resume.

Actions:
- Add or update focused x86 handoff and route-debug coverage for the positive
  selected-`LoadLocal` route.
- Add reachable fail-closed rows for missing, incomplete, drifted, or
  unsupported authoritative prepared compare-join metadata.
- Run the supervisor-delegated proof command and write `test_after.log` when
  code or tests change.
- Record in `todo.md` whether idea 261 can resume.

Completion check:
- Focused x86 proof is green, existing supported shapes are not weakened, and
  `todo.md` states the resume decision for idea 261.
