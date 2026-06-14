# Phase F3 x86 Route 3 LoadLocal Publication Fixture Support

Status: Active
Source Idea: ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md

## Purpose

Repair the x86 proof surface needed to validate the Route 3 selected
`LoadLocal` source-memory agreement facade without synthetic publication
injection.

## Goal

Create or identify one supported x86 fixture path that naturally carries both a
Route 3 `LoadLocal` memory record and a prepared
`PreparedEdgePublication::source_memory_access` row into
`render_agreed_route3_load_local_statement_memory_operand(...)`.

## Core Rule

Do not prove idea 258 by hand-building publications on a route that would
reject them before the statement-memory agreement facade. The fixture must be a
supported backend shape.

## Read First

- `ideas/open/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md`
- `ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md`
- `todo.md` history from idea 258 Step 4.
- `tests/backend/bir/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_short_circuit_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_loop_countdown_test.cpp`
- x86 handoff publication builders and Route 3 memory/source lookup helpers.

## Current Targets

- A supported x86 fixture shape that reaches
  `render_agreed_route3_load_local_statement_memory_operand(...)`.
- Natural selected `LoadLocal` `source_memory_access` publication ownership.
- Focused tests for positive agreement and fail-closed rows needed by idea 258.
- Existing prepared lookup/status and x86 route-debug/handoff-boundary
  contracts.

## Non-Goals

- Do not rewrite x86 addressing, frame layout, register materialization, or
  emitted output policy.
- Do not make prepared `memory_accesses` the semantic authority for Route 3
  selected-memory facts.
- Do not implement generic memory parity beyond the selected `LoadLocal` proof
  surface.
- Do not change RISC-V behavior.
- Do not weaken, delete, or rename existing prepared lookup/status, fallback,
  helper/oracle, route-debug, or x86 output contracts.

## Working Model

- Idea 258 already owns the agreement facade implementation.
- This runbook owns the missing supported fixture/publication surface needed to
  prove that facade.
- Legacy no-publication fallback can remain compatible behavior, but it is not
  positive Route 3/prepared agreement proof.
- A valid fixture must use publication ownership that is already valid for the
  x86 route reaching the statement-memory facade.

## Execution Rules

- Start from the existing Step 4 blocker notes; do not repeat unsupported
  synthetic publication attempts.
- Keep code changes limited to fixture/testability support required to reach
  the selected facade.
- Preserve existing supported fixture semantics and add coverage rather than
  weakening current assertions.
- Use x86-enabled validation for x86-only CTest targets.
- Return to plan-owner if no supported publication route can be created without
  broad target-policy rewrites.

## Steps

### Step 1: Supported Publication Route Contract

Goal: select the smallest supported x86 route that can own a selected
`LoadLocal` source-memory publication and reach the statement-memory agreement
facade.

Actions:
- Re-read the idea 258 Step 4 blocker notes in `todo.md` and avoid the rejected
  synthetic `join_transfers` path.
- Inspect x86 handoff publication builders and existing short-circuit,
  joined-branch, loop-countdown, and local-slot guard fixtures.
- Identify the route where a selected `LoadLocal` publication can be validly
  produced before the facade is called, or record why a tiny fixture helper is
  required.
- Record the accepted route contract in `todo.md`.

Completion check:
- `todo.md` names the supported route, the publication ownership source, and
  the route guards that make it non-synthetic.

### Step 2: Fixture or Helper Implementation

Goal: add the minimal supported fixture/helper wiring needed for tests to reach
the existing agreement facade.

Actions:
- Implement only the fixture/testability support required by Step 1.
- Keep Route 3 agreement semantics in
  `render_agreed_route3_load_local_statement_memory_operand(...)`; do not move
  target policy into test helpers.
- Preserve existing prepared lookup/status, fallback names, route-debug output,
  and x86 operand spelling.
- Record any untestable rejection row explicitly in `todo.md`.

Completion check:
- The supported fixture path reaches the existing agreement facade without
  unsupported publication injection or broad x86 policy rewrites.

### Step 3: Focused Agreement Proof

Goal: prove the positive agreement path and reachable fail-closed rows through
the supported fixture surface.

Actions:
- Add or update focused backend tests for the supported positive agreement
  path.
- Add missing, incomplete, prepared-only, route-only, unsupported, and mismatch
  rows where the selected route can naturally express them.
- Do not count legacy no-publication fallback as positive agreement.
- Run the supervisor-delegated proof command and write `test_after.log` when
  code or tests change.

Completion check:
- Focused x86 proof passes and covers positive agreement plus the reachable
  fail-closed rows without weakening existing contracts.

### Step 4: Hand Back to Idea 258

Goal: decide whether idea 258 can resume or whether the fixture route remains
blocked.

Actions:
- Re-check prepared lookup/status behavior, fallback names, route-debug output,
  and x86 output stability.
- Record in `todo.md` whether idea 258 Step 4 now has a valid proof surface.
- If the proof surface exists and validation is green, request plan-owner
  closure or switch back to idea 258.
- If no supported route exists without broad target-policy rewrites, record the
  exact blocker for supervisor review.

Completion check:
- `todo.md` records either validated resume readiness for idea 258 or the
  remaining testability blocker.
