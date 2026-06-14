# Phase F1 Prepared Publication Status Compatibility Retention

Status: Active
Source Idea: ideas/open/246_phase_f1_prepared_publication_status_compatibility_retention.md

## Purpose

Preserve prepared publication, source-memory, aggregate-stack-source,
current-block publication, route-debug, and helper-oracle status surfaces as
explicit compatibility contracts while route-native diagnostics are introduced
beside them.

## Goal

Make every retained prepared status family visible, owned, and proven before
any later demotion gate treats route-native facts as replacement authority.

## Core Rule

Do not rename, hide, collapse, or delete prepared compatibility statuses in
this plan. Route-native diagnostics may be added only beside retained prepared
rows until independent positive, missing, mismatch, duplicate, fallback, and
wrapper-output behavior is proven.

## Read First

- `ideas/open/246_phase_f1_prepared_publication_status_compatibility_retention.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `src/backend/mir/x86/debug/debug.cpp`
- `src/backend/mir/riscv/codegen/emit.cpp`

## Current Targets And Scope

- Prepared edge-publication source-memory statuses.
- Typed-stack-source and aggregate-stack-source statuses.
- Current-block-entry publication statuses.
- Helper-oracle statuses.
- x86 route-debug strings and `ConsumedPlans` compatibility.
- riscv prepared edge-publication fallback strings and wrapper-visible output.
- Route-native diagnostic naming plans for Route 5 edge/source identity,
  Route 3 memory/source identity, and x86 Route 6 scalar call-use source
  identity.

## Non-Goals

- Do not implement new x86 or riscv adapters.
- Do not delete, hide, privatize, or replace whole prepared lookup groups.
- Do not rename existing status strings or collapse prepared and route-native
  statuses into one ambiguous enum.
- Do not open draft 155 or claim aggregate prepared retirement readiness.
- Do not refresh broad baselines, weaken expected strings, or downgrade
  supported paths.

## Working Model

Prepared status names and helper-oracle outputs remain public compatibility
authority. Route-native rows are diagnostic additions until their own behavior
is independently proven and wrapper output remains stable.

## Execution Rules

- Keep changes narrowly scoped to compatibility diagnostics and tests.
- Prefer adding explicit compatibility rows or assertions over replacing old
  names with route-native labels.
- Treat string equality as insufficient proof of route-native ownership.
- Keep target ABI, layout, register, stack, emission, and wrapper policy in the
  target backends.
- For code-changing steps, run the delegated build and narrow ctest subset and
  write the result to `test_after.log`.
- Before close, require matching before/after regression guard coverage for
  prepared lookup helper, x86 route-debug, and riscv edge-publication tests.

## Ordered Steps

### Step 1: Establish the compatibility status baseline

Goal: Inventory the prepared and route-debug status families that must remain
stable through this plan.

Primary targets:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `src/backend/mir/x86/debug/debug.cpp`
- `src/backend/mir/riscv/codegen/emit.cpp`

Concrete actions:
- Inspect current assertions for `missing_prepared_memory_access`,
  `incomplete_prepared_memory_access`, `unsupported_source_home`,
  `missing_same_width_i32_type`, `missing_destination_gpr_bank`,
  `missing_aggregate_copy_authority`, `publication_unavailable`, and adjacent
  helper-oracle, fallback, and wrapper-output strings.
- Identify which prepared status family owns each compatibility surface.
- Record any existing route-native diagnostic rows and whether they are
  additions or replacements.
- Do not edit implementation files in this step unless the supervisor
  delegates a code-changing packet.

Completion check:
- `todo.md` records the retained status families, their current proof files,
  and any missing compatibility assertions that later steps must add.

### Step 2: Add explicit prepared compatibility proof where gaps exist

Goal: Make retained prepared fallback and helper-oracle behavior visible in
tests or diagnostics without changing status names.

Primary targets:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

Concrete actions:
- Add focused assertions for prepared fallback statuses that are public
  compatibility authority but not yet directly protected.
- Keep expected strings unchanged unless adding a new, separately named
  route-native row beside an existing prepared row.
- Preserve wrapper-visible output for riscv edge-publication cases.

Completion check:
- Narrow prepared/riscv tests pass.
- `todo.md` names the exact compatibility statuses newly protected and records
  the proof command.

### Step 3: Add x86 Route 6 diagnostic naming beside compatibility rows

Goal: Ensure x86 Route 6 scalar call-use source identity diagnostics can be
distinguished from retained `ConsumedPlans` and route-debug compatibility
fallback.

Primary targets:
- `src/backend/mir/x86/debug/debug.cpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Concrete actions:
- Add or expose route-native x86 diagnostic rows only as separately labeled
  evidence beside existing compatibility rows.
- Cover positive, missing, mismatch, duplicate, fallback, and wrapper-output
  behavior when the existing test surface permits it.
- Do not change wrapper authority or prepared fallback behavior.

Completion check:
- x86 route-debug and prepared lookup helper tests pass.
- Existing x86 wrapper-output expectations remain stable.

### Step 4: Add riscv Route 5/Route 3 diagnostic naming beside compatibility rows

Goal: Ensure riscv Route 5 edge/source identity and Route 3 memory/source
identity diagnostics coexist with prepared edge-publication compatibility.

Primary targets:
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Concrete actions:
- Add or expose route-native riscv diagnostic rows only as separately labeled
  evidence beside retained prepared rows.
- Preserve prepared fallback strings and exact emitted wrapper output.
- Keep riscv register, stack, scratch, offset, instruction spelling, and
  source-memory policy target-owned.

Completion check:
- riscv edge-publication and prepared lookup helper tests pass.
- Route-native diagnostic names are visibly separate from prepared fallback
  names.

### Step 5: Prove compatibility retention across x86 and riscv

Goal: Re-prove that retained compatibility diagnostics do not perturb emitted
output or public helper surfaces.

Primary targets:
- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Concrete actions:
- Run the supervisor-delegated narrow proof covering prepared lookup helper,
  x86 route-debug, and riscv edge-publication tests.
- Check wrapper-output assertions for both targets.
- Record any remaining diagnostic-only gaps in `todo.md` instead of expanding
  scope into adapter or deletion work.

Completion check:
- Narrow proof passes and `test_after.log` records the result.
- `todo.md` states whether compatibility retention is ready for close review
  or names blockers.

### Step 6: Close with matched regression evidence

Goal: Decide whether the source idea is complete and ready to archive before
the final prepared field-group demotion gate activates.

Concrete actions:
- Verify every retained prepared status family has an explicit compatibility
  owner and proof path.
- Verify route-native diagnostic rows, if added, are separately named and do
  not replace prepared rows.
- Run or reuse matching before/after regression guard coverage for prepared
  lookup helper, x86 route-debug, and riscv edge-publication tests.
- Do not claim deletion, aggregate retirement, or draft 155 readiness.

Completion check:
- The plan owner can close idea 246 only if source-idea acceptance criteria are
  satisfied and the regression guard passes.
