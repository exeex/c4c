# Edge Publication Lookup Declaration Owner

Status: Active
Source Idea: ideas/open/150_edge_publication_lookup_declaration_owner.md

## Purpose

Give `make_prepared_edge_publication_lookups` a narrow declaration owner so
AArch64 dispatch producer code does not need the broad
`src/backend/prealloc/prepared_lookups.hpp` facade just to call that helper.

## Goal

Move or expose the helper declaration through a narrow owner while preserving
prepared edge-publication behavior and existing true facade users.

## Core Rule

This is declaration ownership cleanup only. Do not change edge-publication
lookup semantics, backend lowering behavior, test expectations, or unsupported
markers.

## Read First

- `ideas/open/150_edge_publication_lookup_declaration_owner.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`

## Current Targets And Scope

- The target helper is `prepare::make_prepared_edge_publication_lookups`.
- The remaining motivating consumer is
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.
- Candidate declaration ownership should be narrower than
  `prepared_lookups.hpp`; prefer an existing edge-publication owner when it can
  own the declaration without becoming an umbrella facade.
- The implementation in `prepared_lookups.cpp` may continue to define the
  helper if that remains the narrowest behavior-preserving route.

## Non-Goals

- Do not rename, wrap, or duplicate `make_prepared_edge_publication_lookups`.
- Do not remove unrelated `PreparedFunctionLookups`, return-chain, RISC-V,
  x86, or owning implementation facade includes.
- Do not introduce a broad umbrella compatibility header under another name.
- Do not expand this into prepared lookup construction rewrites.

## Working Model

`prepared_lookups.hpp` currently owns both aggregate prepared lookup facade
types and the edge-publication lookup helper declarations. The source idea
requires separating the edge-publication helper declaration from that facade so
callers that only need edge-publication lookup construction can include a
narrower declaration owner.

## Execution Rules

- Keep edits behavior-preserving and local to declaration ownership plus
  include repair needed to compile.
- Prefer direct includes for the types used by each declaration or call site;
  do not rely on unrelated transitive includes to hide the old dependency.
- After include changes, re-scan direct `prepared_lookups.hpp` includes and
  explain any remaining direct users.
- Required proof for the implementation slice:
  `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.

## Steps

### Step 1: Map Declaration And Type Dependencies

Goal: Identify the smallest legitimate declaration owner for
`make_prepared_edge_publication_lookups`.

Primary targets:
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:
- Inspect the helper overload declarations and definitions.
- List every type required by the helper signatures:
  `PreparedNameTables`, `PreparedControlFlowFunction`,
  `PreparedValueLocationFunction`, `PreparedValueHomeLookups`,
  `PreparedBirModule`, and `PreparedEdgePublicationLookups`.
- Decide whether an existing header such as `publication_plans.hpp` can own
  the declarations cleanly, or whether a narrowly named new prealloc header is
  needed.
- Confirm the chosen owner does not become a replacement umbrella for
  `prepared_lookups.hpp`.

Completion check:
- The executor can name the chosen owner and the direct includes it needs
  before editing declarations.

### Step 2: Move The Helper Declarations

Goal: Make the narrow owner declare both overloads of
`make_prepared_edge_publication_lookups`.

Primary targets:
- Chosen narrow owner header
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:
- Add the helper declarations to the chosen narrow owner.
- Remove the duplicate declarations from `prepared_lookups.hpp`, or leave only
  a direct include of the narrow owner there when the facade still exports the
  helper for aggregate users.
- Ensure `prepared_lookups.cpp` includes the declaration owner intentionally
  through its existing include graph or with an explicit direct include.
- Keep definitions unchanged except for any include ordering required by the
  new owner.

Completion check:
- The helper is no longer declared only by `prepared_lookups.hpp`, and there
  is exactly one intentional declaration owner.

### Step 3: Update The AArch64 Consumer

Goal: Remove the broad facade include from the motivating AArch64 dispatch
producer when it only needs the edge-publication helper and related narrow
types.

Primary target:
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`

Actions:
- Replace the direct `../../../prealloc/prepared_lookups.hpp` include with
  the chosen narrow owner header if the file no longer needs the facade.
- Add any other direct narrow includes required for names already used in the
  file, such as select-chain lookup helpers.
- Verify the file is not compiling only because another unrelated header
  includes `prepared_lookups.hpp` transitively.

Completion check:
- `dispatch_producers.cpp` does not include `prepared_lookups.hpp` solely for
  `make_prepared_edge_publication_lookups`.

### Step 4: Audit Remaining Facade Includes

Goal: Make sure the declaration-owner change did not hide unresolved facade
dependencies.

Primary targets:
- direct `prepared_lookups.hpp` include sites under `src/backend`
- touched prealloc headers and implementation files

Actions:
- Re-run a repository search for direct `prepared_lookups.hpp` includes.
- Classify each remaining include as aggregate facade use, owning
  implementation use, architecture-specific prepared lookup facade use, or a
  separately recorded blocker.
- Do not remove unrelated includes as part of this idea unless they are
  necessary fallout from the helper declaration move.

Completion check:
- The remaining direct facade includes are justified in `todo.md`, and no
  narrow consumer relies on the old helper declaration owner by accident.

### Step 5: Prove The Slice

Goal: Validate the declaration-owner change without treating narrow proof as a
semantic backend change.

Actions:
- Run `cmake --build --preset default`.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure`.
- If either command fails for reasons unrelated to the slice, record the exact
  failure and blocker in `todo.md`.

Completion check:
- Build and backend CTest proof are recorded in `todo.md`, or a concrete
  blocker is documented for supervisor routing.
