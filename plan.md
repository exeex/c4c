# Source-Producer Same-Block Materialization Owner Runbook

Status: Active
Source Idea: ideas/open/144_source_producer_same_block_materialization_owner.md

## Purpose

Move source-producer and same-block materialization declarations out of the
residual `prepared_lookups.*` facade and into a narrow
source-producer/select-chain owner without changing backend lowering behavior.

## Goal

Make same-block producer facts, publication source-producer lookups, current
block publication consumption queries, and same-block scalar/integer-constant
query helpers available from narrower source-producer ownership while keeping
`PreparedFunctionLookups` as the aggregate wiring point.

## Core Rule

This is an ownership move only. Do not change source-producer semantics,
publication planning, materialization behavior, AArch64 emission policy, call
argument policy, comparison policy, or memory/load-local policy.

## Read First

- `ideas/open/144_source_producer_same_block_materialization_owner.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`

## Current Targets

- same-block producer query facts currently exposed through
  `prepared_lookups.hpp`
- publication source-producer lookup helper declaration
- current-block publication consumption query declarations
- same-block scalar query helper declarations
- same-block integer-constant query helper declarations
- consumers that include `prepared_lookups.hpp` only for these declarations

## Non-Goals

- Do not reopen completed select-chain ownership from idea 137 except for this
  concrete residual declaration dependency.
- Do not move AArch64 materialization, register allocation, scratch, hazard, or
  final emission policy into prealloc.
- Do not replace prepared source-producer facts with local target scans.
- Do not fold call-argument, comparison, or load-local specialized policies
  into this route before the shared same-block owner is stable.
- Do not perform broad residual include cleanup beyond consumers touched by
  this owner move.

## Working Model

- `PreparedFunctionLookups` remains the aggregate that constructs and exposes
  prepared facts.
- Same-block producer and materialization query APIs should have a narrower
  public owner than the residual prepared lookup facade.
- `select_chain_lookups.hpp` is the expected owner for source-producer and
  same-block materialization declarations unless inspection proves a fact
  struct already belongs in `publication_plans.hpp`.
- Publication planning should own only fact structs that already belong to
  publication planning, not target-local materialization or emission policy.

## Execution Rules

- Keep the move behavior-preserving.
- Preserve names, signatures, return behavior, and lookup semantics unless a
  compile-only owner move requires a mechanical namespace/include adjustment.
- Update includes only where the ownership move justifies it.
- Avoid unrelated refactors, formatting churn, helper renames, and expectation
  rewrites.
- For code-changing steps, prove with `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full CTest if source-producer semantics change instead of only
  declaration/include ownership.

## Step 1: Inspect Declaration Surface And Consumers

Goal: map the exact same-block/source-producer surface before moving anything.

Primary targets:

- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`

Actions:

- Identify same-block producer query facts currently declared in
  `prepared_lookups.hpp`.
- Locate the publication source-producer lookup helper declaration and
  definition.
- Locate current-block publication consumption query declarations and
  definitions.
- Locate same-block scalar and integer-constant query helper declarations and
  definitions.
- Search direct callers and include users for each target declaration.
- Decide whether each target belongs in `select_chain_lookups.hpp` or, for
  publication-planning fact structs only, `publication_plans.hpp`.
- Record the target list, consumers, and chosen owners in `todo.md` before
  editing code.

Completion check:

- `todo.md` records the declaration, definition, consumer, and chosen-owner
  map for the current targets.
- No implementation files are edited in this step.

## Step 2: Move Shared Same-Block Source-Producer APIs

Goal: make the shared same-block/source-producer declarations available from
the narrow owner.

Primary targets:

- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:

- Move same-block producer query fact declarations to the selected narrow
  owner.
- Move the publication source-producer lookup helper declaration toward
  `select_chain_lookups.hpp`.
- Move current-block publication consumption query declarations to the selected
  narrow owner.
- Move same-block scalar and integer-constant query helper declarations to the
  selected narrow owner.
- Move definitions only when the selected implementation owner requires it;
  otherwise keep implementation location stable and include the new public
  owner.
- Keep `PreparedFunctionLookups` construction, aggregate fields, and one-pass
  preparation wiring intact.

Completion check:

- Target declarations are no longer publicly owned by the residual
  `prepared_lookups.hpp` facade unless they still require
  `PreparedFunctionLookups`.
- Implementations compile from their selected owner arrangement.
- The move does not introduce local rediscovery scans or target-specific
  shortcuts.

## Step 3: Update Consumers And Narrow Includes

Goal: point consumers at the new owner without doing broad include cleanup.

Actions:

- Update consumers to include the narrow owner for same-block/source-producer
  declarations.
- Remove `prepared_lookups.hpp` includes only when a file no longer needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, or another
  residual prepared lookup API.
- Keep any remaining broad include where the file still names aggregate
  prepared lookup APIs.
- Do not use this step for call-argument, comparison, load-local, or residual
  include cleanup deferred to later ideas.

Completion check:

- Consumers compile against the narrow owner for moved declarations.
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
- No source-producer, publication, materialization, AArch64 emission, call,
  comparison, or memory behavior changed.
- The active source idea acceptance criteria are satisfied or the remaining
  blocker is explicit.
