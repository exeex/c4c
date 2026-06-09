# Stack-Layout Id Lookup Helper Ownership Runbook

Status: Active
Source Idea: ideas/open/143_stack_layout_id_lookup_helpers_owner.md

## Purpose

Move the stack-layout id lookup helpers out of the residual
`prepared_lookups.*` facade and into stack-layout ownership without changing
layout construction or backend lowering behavior.

## Goal

Make `find_frame_slot_by_id` and `find_stack_object_by_id` available from the
stack-layout owner, update consumers to include that owner, and preserve
target-independent behavior.

## Core Rule

This is an ownership move only. Do not change frame layout semantics,
addressing behavior, memory retargeting, comparison behavior, or AArch64
wrapper policy.

## Read First

- `ideas/open/143_stack_layout_id_lookup_helpers_owner.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- existing stack-layout implementation files under
  `src/backend/prealloc/stack_layout/`

## Current Targets

- `find_frame_slot_by_id`
- `find_stack_object_by_id`
- declarations currently exposed through `prepared_lookups.hpp`
- implementations currently owned by `prepared_lookups.cpp`
- consumers that include `prepared_lookups.hpp` only for stack-layout id
  helper declarations

## Non-Goals

- Do not rewrite stack layout construction.
- Do not change frame-address, addressing, memory retargeting, comparison, or
  AArch64 wrapper behavior.
- Do not move target-specific AArch64 frame-address policy into prealloc.
- Do not perform broad prepared-lookup include cleanup beyond consumers touched
  by this helper owner move.
- Do not introduce an umbrella header that hides the old prepared lookup facade
  under a different name.

## Working Model

- `PreparedFunctionLookups` may continue to aggregate and wire broad prepared
  facts.
- Stack-layout id helpers are pure scans over `PreparedStackLayout`, so their
  public owner should be stack layout.
- Consumers should include the narrow stack-layout owner when they only need
  these helper declarations.
- `prepared_lookups.*` should keep only declarations and includes required for
  its remaining aggregate responsibilities.

## Execution Rules

- Keep the move behavior-preserving.
- Prefer an existing stack-layout source file for implementation if repo
  patterns support it; introduce a narrow source file only if needed.
- Update includes only where the helper ownership move justifies it.
- Avoid unrelated refactors, formatting churn, or expectation rewrites.
- For code-changing steps, prove with `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.

## Step 1: Inspect Helper Declarations, Definitions, And Consumers

Goal: map the exact current owner surface before moving anything.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- consumers found by searching for both helper names

Actions:

- Locate the declarations and definitions for `find_frame_slot_by_id` and
  `find_stack_object_by_id`.
- Identify every direct caller and every include that depends on the helper
  declarations.
- Inspect stack-layout source-file conventions to choose the narrowest
  implementation home.
- Record any include or ownership ambiguity in `todo.md` before editing code.

Completion check:

- `todo.md` records the current declaration, definition, consumer, and chosen
  stack-layout implementation owner.
- No implementation files are edited in this step.

## Step 2: Move The Helper API To Stack Layout Ownership

Goal: make stack layout the public and implementation owner for the id lookup
helpers.

Primary targets:

- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- selected stack-layout implementation file
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Move helper declarations from `prepared_lookups.hpp` to
  `stack_layout.hpp`.
- Move helper definitions from `prepared_lookups.cpp` to the selected
  stack-layout implementation file.
- Keep names, signatures, return behavior, and scan semantics unchanged.
- Leave `PreparedFunctionLookups` construction and remaining prepared lookup
  declarations intact.

Completion check:

- The helper declarations are stack-layout-owned.
- The helper definitions compile from a stack-layout implementation owner.
- `prepared_lookups.*` no longer owns these helper declarations or
  definitions.

## Step 3: Update Consumers And Narrow Includes

Goal: point consumers at the stack-layout owner without creating broad include
churn.

Actions:

- Update direct consumers to include
  `src/backend/prealloc/stack_layout/stack_layout.hpp` where they need these
  helpers.
- Remove `prepared_lookups.hpp` includes only when the file no longer needs
  any remaining prepared lookup declarations.
- Keep any include of `prepared_lookups.hpp` that still names
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or other
  residual prepared lookup APIs.
- Avoid unrelated include cleanup deferred to idea 149.

Completion check:

- Consumers compile against the stack-layout owner for stack id helpers.
- No consumer loses required aggregate prepared lookup declarations.
- The diff does not change lowering behavior.

## Step 4: Prove And Record The Ownership Move

Goal: validate the behavior-preserving ownership move and leave executor state
ready for supervisor review.

Actions:

- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- If validation fails, record the exact failure and the next narrowed fix in
  `todo.md`.
- If validation passes, record the proof commands and result in `todo.md`.

Completion check:

- Build proof and backend CTest proof are recorded in `todo.md`.
- No AArch64 wrapper, frame-address, addressing, memory retargeting, or
  comparison behavior changed.
- The active source idea acceptance criteria are satisfied or the remaining
  blocker is explicit.
