# AArch64 Edge-Copy Typed Stack-Source Prepared Authority Runbook

Status: Active
Source Idea: ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md

## Purpose

Move the same-width I32 typed stack-source edge-copy publication path onto
`PreparedTypedStackSourcePublication` without expanding publication ordering,
dispatch-family scheduling, or shared prealloc responsibilities.

## Goal

Make `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` consume
`PreparedTypedStackSourcePublication` when the matching prepared edge
publication fact is available, while keeping AArch64 load/copy/register
emission target-local.

## Core Rule

Do not repair this by adding testcase-shaped fallbacks, weakening expectations,
or threading `PreparedEdgePublication` into
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp` only to satisfy the
old route.

## Read First

- `ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- Prepared edge publication and typed stack-source publication definitions and
  query helpers near the existing prealloc publication surfaces.
- Existing focused tests that exercise typed stack-source edge-copy
  publication, same-width I32 publication, and dispatch edge copies.

## Current Targets

- Primary implementation target:
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- Shared/prepared fact target only if the existing API must be consumed more
  directly:
  prepared edge publication lookup surfaces that already define or expose
  `PreparedEdgePublication` and
  `prepare_same_width_i32_stack_source_publication`.
- Tests:
  the narrow backend or ctest subset selected by the supervisor for the touched
  typed stack-source edge-copy path.

## Non-Goals

- Do not rewrite publication ordering, edge-copy scheduling, or the shared
  publication-plan model.
- Do not move AArch64 load mnemonics, register spelling, extension rendering,
  copy instruction choice, or machine-record construction into shared code.
- Do not weaken supported-path tests, diagnostics, unsupported markers, or
  expected output contracts.
- Do not broaden into dispatch-family contraction, call publication cleanup, or
  general memory/address cleanup.

## Working Model

`dispatch_publication.cpp` already consumes prepared store-source publication
plans but does not own the `PreparedEdgePublication*` needed by
`prepare_same_width_i32_stack_source_publication`. The likely correct owner is
the edge-copy emission path, where `PreparedEdgePublication` is already close
to the copy decision and target-local machine record construction can remain in
AArch64 code.

## Execution Rules

- Keep each packet small enough to prove with a focused build or ctest subset.
- Preserve prepared facts as authority inputs; do not locally re-derive the
  same-width I32 typed stack-source decision after the prepared fact is
  available.
- Preserve target-local emission after authority selection: load/copy/register
  rendering stays in AArch64 code.
- Record blockers explicitly in `todo.md` if the prepared fact is unavailable
  at the needed call site.
- Acceptance-sized code slices need regression guard logs before closure.

## Ordered Steps

### Step 1: Map Edge-Copy Publication Inputs

Goal: Confirm where edge-copy emission already receives or can access
`PreparedEdgePublication`.

Primary target: `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

Actions:

- Inspect all `dispatch_edge_copies.cpp` call sites and helper signatures that
  receive `PreparedEdgePublication`, `PreparedEdgePublication*`, or related
  publication facts.
- Trace the same-width I32 stack-source path from prepared edge publication
  data to the current AArch64 load/copy emission.
- Identify the narrow insertion point for
  `prepare_same_width_i32_stack_source_publication`.
- Note any missing authority input as a blocker instead of manufacturing a
  fallback.

Completion check:

- `todo.md` records the selected insertion point, the relevant prepared fact
  source, and whether implementation can proceed without changing
  `dispatch_publication.cpp`.

### Step 2: Consume Prepared Typed Stack-Source Authority

Goal: Route the same-width I32 stack-source edge-copy publication through
`PreparedTypedStackSourcePublication`.

Primary target: `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

Actions:

- Call or otherwise consume
  `prepare_same_width_i32_stack_source_publication` at the insertion point
  proven by Step 1.
- Use `PreparedTypedStackSourcePublication` to decide the matching prepared
  stack-source publication path.
- Leave load selection, register view conversion, extension policy rendering,
  copy instruction emission, and machine record construction target-local.
- Avoid adding named-testcase branches or shape-only special cases.

Completion check:

- The same-width I32 typed stack-source edge-copy path uses the prepared fact
  as authority and no longer locally re-derives the same decision.
- Focused build proof passes for the touched backend target or test binary.

### Step 3: Add Focused Proof

Goal: Prove the migrated path without weakening existing behavior.

Primary target: existing focused backend tests for typed stack-source
edge-copy publication.

Actions:

- Add or adjust the narrowest test that exercises the same-width I32
  stack-source edge-copy publication path touched by Step 2.
- Keep expected supported-path behavior at least as strong as before.
- If a nearby missing-authority case remains unsupported, document it in
  `todo.md` as a blocker or follow-up rather than hiding it behind fallback
  logic.

Completion check:

- The delegated proof command passes and `test_after.log` captures the result
  when requested by the supervisor.

### Step 4: Acceptance Sweep

Goal: Verify the completed route still matches the source idea and has enough
proof for closure consideration.

Actions:

- Review the diff for named-case shortcuts, expectation downgrades, and shared
  code ownership drift.
- Run the supervisor-selected broader proof or regression guard before the
  source idea is considered closable.
- Ensure any remaining blockers are explicit in `todo.md`.

Completion check:

- The active route satisfies the source idea acceptance criteria or has a
  clearly recorded blocker that prevents closure.
