# Addressing Lookup Ownership Cleanup Runbook

Status: Active
Source Idea: ideas/open/139_addressing_lookup_ownership_cleanup.md

## Purpose

Move memory-access, address-materialization, and frame-address-offset lookup
ownership toward the addressing/frame domain instead of exposing those helpers
only through the generic prepared lookup facade.

## Goal

Give reusable target-neutral addressing facts an addressing/frame-domain public
boundary while preserving the existing cached `PreparedFunctionLookups`
construction path for consumers that need aggregate lookup access.

## Core Rule

This is an ownership-boundary cleanup, not an address-lowering or stack-layout
semantic change. Do not replace prepared addressing facts with local scans,
name matching, or AArch64-only rediscovery.

## Read First

- `ideas/open/139_addressing_lookup_ownership_cleanup.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- Known AArch64 consumers listed in the source idea

## Current Targets

- `PreparedAddressMaterializationLookups`
- `PreparedMemoryAccessLookups`
- `make_prepared_address_materialization_lookups`
- `make_prepared_memory_access_lookups`
- `find_indexed_prepared_memory_access*`
- `collect_prepared_address_materializations_for_block`
- `find_indexed_prepared_frame_address_offset_for_value`
- `find_indexed_prepared_frame_address_offset_for_value_id`
- `find_prepared_global_load_access`
- `find_prepared_same_block_global_load_access`
- `PreparedFunctionLookups::address_materializations`
- `PreparedFunctionLookups::memory_accesses`

## Non-Goals

- Do not change AArch64 addressing modes, TLS/global relocation behavior,
  stack-frame layout, or store-source publication semantics.
- Do not redesign `publication_plans.*`; it may consume addressing lookup
  helpers but should not become a new owner for unrelated behavior.
- Do not remove cached lookup construction or make hot consumers rebuild the
  same maps per query.
- Do not weaken backend expectations or mark supported paths unsupported.

## Working Model

- `addressing.hpp` should own direct prepared address and memory-access record
  declarations plus public query declarations that are clearly addressing
  facts.
- `stack_layout/stack_layout.hpp` should own frame-address-offset query
  declarations when the query is frame-domain specific.
- `prepared_lookups.*` may continue to build cached lookup maps and wire them
  into `PreparedFunctionLookups`.
- AArch64 consumers should include the narrowest domain header they need when
  they only query addressing/frame facts.

## Execution Rules

- Prefer declaration-boundary moves before definition moves.
- Keep builder definitions in `prepared_lookups.cpp` unless moving them can be
  done without widening dependencies or duplicating lookup construction.
- After code-changing steps, run `cmake --build --preset default`.
- After the final cleanup step, run
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full `ctest --test-dir build -j --output-on-failure` if the diff
  changes address/materialization semantics, global access matching, or frame
  slot resolution.

## Step 1: Inspect current addressing lookup ownership and dependencies

Goal: map every relevant declaration, definition, constructor path, include,
and consumer before moving ownership boundaries.

Actions:

- Locate all current declarations and definitions for the target lookup types,
  builders, collectors, and query helpers.
- Identify which helpers are pure addressing facts, which are frame-domain
  facts, and which still require the broad prepared lookup aggregate.
- Inspect all known AArch64 consumers and any other direct consumers found by
  search.
- Decide the Step 2 declaration boundary without editing implementation files.

Completion check:

- `todo.md` records the locations found, dependency constraints, and the exact
  declaration-boundary move to perform next.
- No build is required for this inspection-only step.

## Step 2: Move addressing/frame lookup declarations to domain headers

Goal: expose reusable addressing and frame lookup query declarations from the
domain headers while keeping cached construction wiring intact.

Primary targets:

- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`

Actions:

- Move address-materialization and memory-access lookup type/helper
  declarations into `addressing.hpp` where they belong to reusable addressing
  facts.
- Move frame-address-offset query declarations into
  `stack_layout/stack_layout.hpp` when they are frame-domain specific.
- Keep `PreparedFunctionLookups` aggregate fields and builder wiring visible
  from `prepared_lookups.hpp` as needed.
- Add forward declarations or narrow includes only where they preserve the
  dependency direction.

Completion check:

- Domain headers provide the intended public declaration boundary.
- `prepared_lookups.hpp` no longer needs to be the only public owner for those
  addressing/frame declarations.
- `cmake --build --preset default` succeeds.

## Step 3: Adjust consumers to the narrowest stable ownership boundary

Goal: make consumers discover addressing/frame facts through the new domain
headers without changing lookup semantics.

Primary targets:

- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/publication_plans.*`
- Known AArch64 consumers listed in the source idea

Actions:

- Replace broad prepared-lookup includes with `addressing.hpp` or
  `stack_layout/stack_layout.hpp` where consumers only need those facts.
- Keep broad `prepared_lookups.hpp` includes where a file still uses
  non-addressing prepared lookup aggregate APIs.
- Verify AArch64 memory/global paths continue to query prepared facts and do
  not rescan BIR, frame slots, globals, or producers locally.

Completion check:

- Consumer includes reflect the new owner boundary.
- No consumer locally rebuilds addressing lookup maps or replaces prepared
  addressing queries with scans.
- `cmake --build --preset default` succeeds.

## Step 4: Validate boundary cleanup and close readiness

Goal: prove the ownership cleanup is complete under the source idea without
semantic drift.

Actions:

- Search for leftover addressing/frame query declarations that still live only
  under `prepared_lookups.hpp`.
- Search for duplicate public helper names split only by owner.
- Inspect `PreparedFunctionLookups::address_materializations` and
  `PreparedFunctionLookups::memory_accesses` construction to confirm cached
  access remains intact.
- Run the backend proof command.

Completion check:

- Acceptance criteria from the source idea are satisfied.
- `cmake --build --preset default` succeeds.
- `ctest --test-dir build -R '^backend_' --output-on-failure` succeeds.
- Full `ctest --test-dir build -j --output-on-failure` is run if the final
  diff changes address/materialization semantics, global access matching, or
  frame slot resolution.
