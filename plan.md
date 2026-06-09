# Call-Plan Lookup Ownership Cleanup Runbook

Status: Active
Source Idea: ideas/open/138_call_plan_lookup_ownership_cleanup.md

## Purpose

Move call-plan lookup indexes and query helpers out of the broad prepared
lookup facade and into a call-domain declaration boundary while preserving
existing prepared call semantics.

## Goal

Make call-plan lookup ownership discoverable from the call domain without
forcing consumers to rebuild indexes or rediscover call facts locally.

## Core Rule

This is an API ownership cleanup. Do not change ABI classification, call
lowering semantics, call-boundary instruction selection, preservation route
calculation, or AArch64 register handling.

## Read First

- `ideas/open/138_call_plan_lookup_ownership_cleanup.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/module.hpp`

## Current Targets

- `PreparedCallPlanLookups`
- outgoing stack argument area lookup helpers
- prior preserved value lookup helpers
- preservation republication lookup helpers
- prepared call-boundary effect lookup helpers
- `PreparedFunctionLookups::call_plans` aggregate construction
- known call-domain and AArch64 consumers listed in the source idea

## Non-Goals

- Do not rework `PreparedCallPlan`, argument/result planning, variadic wrapper
  behavior, or preservation semantics.
- Do not move target-local call emission policy into prealloc.
- Do not split runtime-helper planning.
- Do not replace prepared call facts with local scans in AArch64 codegen.
- Do not weaken expectations, mark supported paths unsupported, or use printer
  rewrites as proof.

## Working Model

- The call domain should own call-plan lookup declarations or an explicit
  call-owned re-export boundary.
- `PreparedFunctionLookups` may continue to aggregate call-plan lookup caches
  for cheap per-function access.
- `prepared_lookups.*` may keep broad aggregate wiring when needed, but it
  should no longer be the only public owner for call-plan lookup APIs.

## Execution Rules

- Keep each step behavior-preserving unless a tiny helper extraction is needed
  to break a dependency cycle.
- Prefer moving declarations, includes, and construction wiring over changing
  data shape.
- After code-changing steps, run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure` before
  treating the slice as acceptance-ready.
- Escalate to full `ctest --test-dir build -j --output-on-failure` if call
  semantics, ABI binding fields, preservation route calculation, or
  call-boundary effect construction changes.

## Ordered Steps

### Step 1: Inspect current call lookup ownership and dependencies

Goal: map where call-plan lookup types, builders, and query helpers are
declared, defined, constructed, and consumed.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/module.hpp`

Actions:

- Identify every `PreparedCallPlanLookups` declaration, constructor path, and
  helper listed in the source idea.
- Identify include dependencies that block moving or re-exporting the lookup
  boundary from `calls.hpp` or another call-owned header.
- Identify all consumers that include `prepared_lookups.hpp` only for
  call-plan lookup APIs.
- Choose the smallest ownership move that avoids circular includes.

Completion check:

- The executor can name the intended call-owned declaration boundary, the
  files that must change, and any dependency cycle that requires a helper
  extraction.

### Step 2: Move or re-export call lookup declarations through call ownership

Goal: make call-plan lookup types and query helpers available from a call-owned
boundary instead of only from `prepared_lookups.hpp`.

Primary targets:

- `src/backend/prealloc/calls.hpp`
- optional call-owned companion header if `calls.hpp` would create cycles
- `src/backend/prealloc/prepared_lookups.hpp`

Actions:

- Move or re-export `PreparedCallPlanLookups` and the call-plan query helper
  declarations through the selected call-owned boundary.
- Keep names stable unless a narrow rename is necessary to clarify ownership.
- Leave broad prepared lookup aggregate declarations in place only when they
  are still genuinely broad aggregate wiring.
- Avoid changing struct fields or query behavior unless required by the move.

Completion check:

- Call-plan lookup declarations are reachable from a call-owned boundary, and
  `prepared_lookups.hpp` is no longer the sole owner of those public APIs.

### Step 3: Rewire definitions and aggregate construction

Goal: keep lookup construction cheap and behavior-preserving after the ownership
move.

Primary targets:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/module.hpp`

Actions:

- Move or keep definitions in the smallest compilation unit that matches the
  new declaration boundary without creating dependency cycles.
- Preserve `PreparedFunctionLookups::call_plans` aggregate construction.
- Preserve outgoing stack, prior-preservation, preservation republication, and
  call-boundary effect lookup indexing behavior.
- Run `cmake --build --preset default`.

Completion check:

- The build succeeds, and no consumer must rebuild call lookup maps manually.

### Step 4: Update known consumers to include the domain boundary

Goal: make consumers depend on the call-owned lookup boundary where they use
call-plan lookup facts.

Primary targets:

- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `src/backend/prealloc/prepared_printer/functions.cpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`

Actions:

- Replace broad prepared lookup includes with the call-owned boundary when a
  file only needs call-plan lookup facts.
- Keep broader includes where the file still consumes non-call lookup groups.
- Verify AArch64 call lowering continues to consume prepared call facts rather
  than duplicating discovery logic.
- Run `cmake --build --preset default`.

Completion check:

- Known call consumers compile through the new ownership boundary without local
  scans or duplicated preservation, outgoing-stack, or call-boundary discovery.

### Step 5: Prove backend behavior and review ownership quality

Goal: validate that the cleanup preserved behavior and did not drift into
semantic call lowering changes.

Actions:

- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Inspect the diff for forbidden semantic changes, expectation weakening, or
  testcase-shaped shortcuts.
- Escalate to full `ctest --test-dir build -j --output-on-failure` only if the
  implementation changed call semantics, ABI binding fields, preservation route
  calculation, or call-boundary effect construction.

Completion check:

- Build and backend proof are fresh, call-plan lookup ownership is call-domain
  discoverable, and the diff remains an API ownership cleanup.
