# Active Plan: Route 6 x86 Scalar Source Route-Debug Row

Status: Active
Source Idea: ideas/open/214_route6_x86_scalar_source_route_debug_row.md

## Purpose

Activate the narrow Route 6 x86 diagnostic replacement described by the source
idea: replace exactly one scalar source agreement route-debug row while keeping
`ConsumedPlans`, prepared debug output, expected strings, and wrapper behavior
authoritative.

Goal: make the x86 route-debug surface report the same scalar source agreement
through route-native Route 6 evidence that the prepared-backed path already
accepts.

## Core Rule

Replace one named route-debug row only. Do not migrate x86 call wrappers, whole
call plans, call printer output, direct-call/helper oracles, ABI policy, or
`ConsumedPlans` ownership.

## Read First

- `ideas/open/214_route6_x86_scalar_source_route_debug_row.md`
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`
- `src/backend/mir/x86/x86.hpp`
- `src/backend/mir/x86/debug/debug.cpp`
- `tests/backend/bir/backend_x86_route_debug_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- The existing x86 Route 6 scalar agreement helper:
  `find_consumed_scalar_i32_call_argument_source(...)` in
  `src/backend/mir/x86/x86.hpp`.
- The x86 route summary/trace renderer in
  `src/backend/mir/x86/debug/debug.cpp`.
- The exact route-debug expected strings in
  `tests/backend/bir/backend_x86_route_debug_test.cpp`.
- The existing helper/fallback proof around x86 `ConsumedPlans` in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Non-Goals

- Do not broaden Route 6 beyond one scalar argument-source debug row.
- Do not change ABI placement, call wrapper kind, clobbers, outgoing stack
  sizing, value homes, move bundles, aggregate transport, or emitted output.
- Do not remove prepared call-plan fallback or public prepared debug authority.
- Do not rewrite expected strings as a baseline refresh.
- Do not downgrade unsupported/supported-path contracts.

## Working Model

`ConsumedPlans` already carries both prepared call-plan data and an optional
`Route6CallUseSourceIndex`. The only route-native source accepted for this
plan is the scalar `i32` call-argument source accepted by
`find_consumed_scalar_i32_call_argument_source(...)`, and only when it agrees
with the prepared argument source id. The route-debug row may observe that
agreement; all wrapper, call-plan, ABI, movement, and fallback policy remains
prepared/target-owned.

## Execution Rules

- Treat absent, invalid, duplicate/conflict, mismatch, and no-`ConsumedPlans`
  cases as prepared fallback or fail-closed behavior, matching current
  semantics.
- Keep expected-string changes limited to the one selected route-debug row and
  justify them with route-native semantic proof.
- Keep helper names and public debug contracts stable unless a rename is
  required by the one-row replacement.
- For every code-changing step, run a fresh build or targeted CTest proof.
- Escalate to broader validation if implementation touches shared helpers,
  wrapper emission, prepared printer code, or expected strings outside the
  selected row.

## Steps

### Step 1: Pin the Selected Debug Row

Goal: identify the exact summary/trace row that will be replaced and the
fixtures that prove it.

Primary target: `tests/backend/bir/backend_x86_route_debug_test.cpp`.

Actions:

- Inspect the current x86 route summary/trace rows for the scalar Route 6
  agreement case.
- Map the row back to `debug.cpp` and the existing `ConsumedPlans` helper in
  `x86.hpp`.
- Record the positive, absent, invalid, duplicate/conflict, mismatch, fallback,
  and wrapper no-change proof points in `todo.md`.
- Do not edit implementation while the target row is still ambiguous.

Completion check:

- `todo.md` names the exact row, fixture, expected text owner, and minimal test
  subset for the implementation packet.

### Step 2: Add Route-Native Row Selection

Goal: make the selected route-debug row read Route 6 scalar source agreement
when the existing x86 agreement helper accepts it.

Primary target: `src/backend/mir/x86/debug/debug.cpp`.

Actions:

- Thread only the data needed to ask the existing `ConsumedPlans`/Route 6
  scalar source agreement question.
- Emit the route-native row only for the named scalar source agreement case.
- Preserve prepared-backed output for absent Route 6 facts, invalid
  references, duplicate/conflicting facts, source-id mismatch, and compatibility
  fallback.
- Avoid moving call-plan, ABI, wrapper, or emission policy into route-debug.

Completion check:

- The positive case reports the same scalar source agreement as the prepared
  path, and negative cases still fall back or fail closed exactly as before.

### Step 3: Prove Debug Text and Fallbacks

Goal: make byte-stable route-debug expectations prove the one-row replacement
without weakening surrounding contracts.

Primary target: `tests/backend/bir/backend_x86_route_debug_test.cpp`.

Actions:

- Add or adjust only the expected strings required for the one selected row.
- Cover positive route-native agreement plus absent, invalid,
  duplicate/conflict, mismatch, and `ConsumedPlans` compatibility behavior.
- Keep prepared printer/debug output and unrelated route-debug rows unchanged.

Completion check:

- The targeted route-debug test fails before the implementation when the row is
  still prepared-backed, and passes after the one-row replacement.

### Step 4: Prove Wrapper No-Change Behavior

Goal: show that x86 wrapper behavior did not contract or migrate while the
debug row changed.

Primary targets:

- `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_lir_test.cpp`
- adjacent x86 wrapper tests selected by the supervisor

Actions:

- Run the delegated wrapper proof subset after the debug-row change.
- If wrapper output changes, revert or narrow the implementation instead of
  accepting the row replacement as progress.
- Do not use wrapper expectation rewrites as evidence.

Completion check:

- The delegated wrapper subset is green with no output or expectation changes.

### Step 5: Acceptance Handoff

Goal: leave the slice ready for supervisor acceptance or reviewer scrutiny.

Actions:

- Update `todo.md` with the final proof commands and results.
- Confirm only `plan.md`, `todo.md`, the selected debug implementation, and
  targeted tests changed.
- Flag any expected-string edit outside the named row as a blocker.

Completion check:

- The slice has fresh targeted proof, no testcase-overfit signs, and no broad
  Route 6/x86 wrapper migration.
