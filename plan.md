# Edge-Copy Facade Split Runbook

Status: Active
Source Idea: ideas/open/140_edge_copy_facade_split.md

## Purpose

Separate reusable edge publication, source, home, and move-bundle facts from
current-block join parallel-copy routing conveniences that encode
AArch64-shaped emission details.

## Goal

Keep target-neutral prepared edge-copy facts reusable for future backends while
moving or clearly naming target-facing routing conveniences around AArch64 join
parallel-copy emission.

## Core Rule

This is a facade ownership split, not a control-flow, out-of-SSA, dispatch, or
memory lowering rewrite. Do not replace prepared edge facts with predecessor
rescans, BIR rediscovery, local value-location scans, or named-case shortcuts.

## Read First

- `ideas/open/140_edge_copy_facade_split.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`

## Current Targets

Shared facts to keep or clarify:

- `PreparedEdgePublicationLookups`
- `PreparedMoveBundleLookups`
- `find_indexed_prepared_edge_publications`
- `find_unique_indexed_block_entry_parallel_copy_edge_publication`
- `prepare_edge_copy_source_facts`

Routing-oriented surfaces to split or rename:

- `prepare_current_block_join_parallel_copy_source_facts`
- `prepare_current_block_join_parallel_copy_instruction_routing`
- `prepare_same_width_i32_stack_source_publication`
- `prepare_aggregate_stack_source_authority`
- fields on `PreparedCurrentBlockJoinParallelCopySourceFact`
- fields on `PreparedCurrentBlockJoinParallelCopyInstructionRouting`

## Non-Goals

- Do not delete reusable edge publication, predecessor/successor, destination
  home, source producer, move-bundle, or parallel-copy authority facts.
- Do not move AArch64 register spelling, scratch-register selection, final
  instruction forms, or concrete emission policy into shared prealloc code.
- Do not rewrite out-of-SSA parallel-copy generation.
- Do not change control-flow semantics or predecessor/successor transfer
  identity.
- Do not weaken backend expectations or mark supported edge-copy cases
  unsupported.

## Working Model

- `control_flow.hpp`, `value_locations.hpp`, and `publication_plans.hpp` should
  expose target-neutral prepared edge facts when those facts are reusable
  outside AArch64.
- AArch64-only or target-facing current-block join routing conveniences should
  either move closer to AArch64 consumers or be named as target-facing
  conveniences.
- `prepared_lookups.*` may continue to build cached lookup maps and wire shared
  lookup aggregates, but it should not be the only public owner for reusable
  edge-copy facts when a narrower domain header is available.
- AArch64 dispatch and memory consumers must continue to use prepared facts
  rather than rebuilding edge/source/home facts locally.

## Execution Rules

- Inspect and classify every target surface before moving declarations or
  definitions.
- Prefer declaration-boundary and naming cleanup before definition moves.
- Keep behavior unchanged unless the source idea explicitly requires a facade
  split that changes ownership or naming.
- Keep reusable edge fact APIs available to non-AArch64 backends.
- After code-changing steps, run `cmake --build --preset default`.
- After the final cleanup step, run
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full `ctest --test-dir build -j --output-on-failure` if the diff
  changes edge publication semantics, parallel-copy authority, or control-flow
  transfer construction.

## Step 1: Inspect current edge-copy facade ownership and dependencies

Goal: map the current shared edge facts, target-facing routing conveniences,
builder paths, includes, and AArch64 consumers before changing ownership.

Actions:

- Locate all declarations and definitions for the current targets listed above.
- Classify each type, field, and helper as one of:
  - reusable target-neutral edge fact
  - target-facing routing convenience
  - cached lookup construction or aggregate wiring
- Inspect all known AArch64 consumers and any other direct consumers found by
  search.
- Identify which routing-oriented surfaces can move target-local, which should
  be renamed as target-facing, and which must remain shared for now.
- Decide the Step 2 facade boundary without editing implementation files.

Completion check:

- `todo.md` records declaration locations, definition locations, construction
  wiring, consumer include constraints, and the exact Step 2 split/rename
  boundary.
- No build is required for this inspection-only step.

## Step 2: Move or rename routing-oriented facade declarations

Goal: make public declarations distinguish reusable edge facts from
target-facing current-block join routing conveniences.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/control_flow.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- AArch64 dispatch/memory headers if routing declarations move target-local

Actions:

- Keep reusable edge publication and move-bundle query declarations available
  from shared domain headers.
- Move target-facing current-block join routing declarations closer to AArch64
  consumers when dependencies allow.
- If a helper must stay shared temporarily, rename or document it as
  target-facing so it is not presented as a generic edge fact.
- Preserve `PreparedFunctionLookups` aggregate wiring and cached construction
  access.

Completion check:

- The public declaration boundary clearly separates shared edge facts from
  target-facing routing conveniences.
- AArch64 consumers still compile against the new or clarified facade.
- `cmake --build --preset default` succeeds.

## Step 3: Adjust definitions and consumers to the split facade

Goal: align definitions and includes with the new ownership boundary without
changing prepared edge-copy semantics.

Primary targets:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/prologue.cpp`

Actions:

- Move definitions only when the dependency direction remains clean and cached
  lookup construction is not duplicated.
- Replace broad includes with narrower shared or AArch64-local headers where
  consumers only need the split facade.
- Keep broad prepared lookup includes where files still use aggregate lookup
  APIs or unrelated prepared facts.
- Verify no consumer starts rescanning predecessors, BIR producers, or local
  value locations in place of prepared edge facts.

Completion check:

- Definitions and consumers match the new facade boundary.
- AArch64 edge-copy and memory paths still use prepared edge facts.
- `cmake --build --preset default` succeeds.

## Step 4: Validate edge-copy facade split and close readiness

Goal: prove the source idea is satisfied without semantic drift or broad
dispatch/memory rewrites.

Actions:

- Search for leftover routing-oriented declarations that still appear as
  generic shared edge facts.
- Search for duplicate helper names or wrappers that preserve the old mixed
  facade under a new name.
- Inspect AArch64 edge-copy and memory consumers for predecessor rescans, BIR
  rediscovery, local source/home scans, or target policy leakage into shared
  prealloc code.
- Run the required backend proof command.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `cmake --build --preset default` succeeds.
- `ctest --test-dir build -R '^backend_' --output-on-failure` succeeds.
- Full `ctest --test-dir build -j --output-on-failure` is run if edge
  publication semantics, parallel-copy authority, or control-flow transfer
  construction changed.
