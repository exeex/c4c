# BIR Route Header Split Runbook

Status: Active
Source Idea: ideas/open/530_bir_route_header_split_after_body_moves.md

## Purpose

Split BIR route declarations out of `src/backend/bir/bir.hpp` only where a
narrow header boundary reduces coupling after the route body owners have
stabilized.

## Goal

Create focused route declaration headers without moving implementation bodies,
changing public names, or forcing broad consumers to include a pile of new
route headers.

## Core Rule

This is behavior-preserving header movement only. Do not change route
semantics, public signatures, namespaces, enum values, or the ownership of
implementation bodies.

## Read First

- `ideas/open/530_bir_route_header_split_after_body_moves.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_private.hpp`
- `src/backend/bir/bir_route_facade.cpp`
- `src/backend/bir/bir_route1.cpp`
- `src/backend/bir/bir_route2.cpp`
- `src/backend/bir/bir_route3_memory.cpp`
- `src/backend/bir/bir_route4_publication.cpp`
- `src/backend/bir/bir_route5_publication.cpp`
- `src/backend/bir/bir_route6_call_publication.cpp`
- `src/backend/bir/bir_route7_comparison.cpp`
- `src/backend/bir/bir_route8.cpp`
- Include users under `src/backend/bir/` and direct-source BIR tests in
  `tests/backend/bir/CMakeLists.txt`
- `.codex/skills/c4c-clang-tools/` for declaration, reference, and include
  evidence before selecting header boundaries

## Current Targets

- Route declaration clusters currently in `src/backend/bir/bir.hpp`
- Candidate focused route headers under `src/backend/bir/`
- Include sites that can depend on a narrower route header instead of the
  entire BIR public header
- Build-system or direct-source test wiring only if include churn exposes a
  missing translation unit dependency

## Non-Goals

- Do not move implementation bodies.
- Do not split or move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not create a new catch-all route monolith.
- Do not change public API names, namespaces, signatures, enum values, or
  behavior.
- Do not make route declaration movement depend on fragile forward
  declarations for complete-type containers such as `std::vector<Function>`.
- Do not combine this idea with memory-provenance or local-array semantic-GEP
  header-readiness work from later ideas.

## Working Model

- `bir.hpp` remains the broad compatibility include for core BIR data model
  types and any consumers that genuinely need the full surface.
- New route headers should be justified by caller/reference evidence, not by
  mechanically evacuating every route-looking declaration.
- Header boundaries should follow already-stabilized body owners where that
  improves include direction.
- A split is not progress if consumers immediately need to include many new
  route headers to recover the old surface.

## Execution Rules

- Start with a mapping-only packet. Record declaration clusters, complete-type
  dependencies, current include users, and candidate boundaries in `todo.md`
  before editing headers.
- Prefer clang-backed symbol and type-reference queries over manual large-file
  inspection when mapping declarations and consumers.
- Move declarations in small groups that can be proved by build plus focused
  backend route tests.
- Keep `bir.hpp` as a stable aggregator when compatibility or complete-type
  requirements require it.
- Treat any semantic diff, body movement, test expectation weakening, or public
  signature change as a blocker.
- Escalate to supervisor for broader backend proof if multiple route headers
  move or include churn reaches LIR-to-BIR or prepared-printer consumers.

## Step 1: Map Route Header Boundaries

Goal: identify which route declarations can move safely and which must remain
in `bir.hpp`.

Primary target: route declarations in `src/backend/bir/bir.hpp` and their
current consumers.

Actions:

- Use clang-backed symbol, caller/callee, and type-reference queries to map
  route declaration clusters in `bir.hpp`.
- Record complete-type dependencies involving `Value`, `Inst`, `Block`,
  `Function`, `Module`, `MemoryAddress`, and route index containers.
- Inspect current include sites under `src/backend/bir/` and direct-source BIR
  tests that rely on the broad `bir.hpp` surface.
- Decide whether the first split should create one focused route header or no
  movement because coupling is not reduced.
- Record candidate header name, declarations to move, declarations to leave,
  required include-site changes, and focused proof recommendations in
  `todo.md`.

Completion check:

- `todo.md` contains the mapped declaration clusters, selected first boundary,
  non-moved declarations, dependency risks, include-site plan, and proof
  command recommendation.
- No implementation or header files are edited in this mapping step.

## Step 2: Introduce One Narrow Route Header

Goal: move the first confirmed declaration cluster into a focused header while
preserving the public compatibility surface.

Primary target: a new or existing focused route header under
`src/backend/bir/`, plus `src/backend/bir/bir.hpp` aggregator includes as
needed.

Actions:

- Create the selected focused header only for the declaration cluster approved
  by Step 1.
- Move declarations without changing spelling, namespace, signatures, enum
  values, or behavior.
- Add minimal includes or forward declarations needed for the moved
  declarations to compile.
- Keep `bir.hpp` able to serve existing broad include users unless Step 1
  proves a direct include-site replacement is safer.
- Do not move any `.cpp` implementation body as part of the header split.
- Run the supervisor-delegated proof command exactly and write results to
  `test_after.log`.

Completion check:

- Build proof passes.
- Focused backend route tests chosen by the supervisor pass.
- Diff shows declaration/header/include movement only, with no body movement,
  semantic changes, or public API changes.

## Step 3: Replace Safe Include Sites

Goal: use the new narrow header only where it reduces dependency pressure
without spreading include burden.

Primary target: implementation files and tests identified by Step 1 as safe
direct users of the focused route surface.

Actions:

- Replace broad `bir.hpp` includes only at sites that need the focused route
  declarations and do not require the full BIR model surface.
- Keep broad includes where replacing them would require several route headers
  or fragile forward declarations.
- Avoid changing source ordering, build target membership, or behavior except
  for required include hygiene.
- Run the supervisor-delegated proof command exactly and write results to
  `test_after.log`.

Completion check:

- Build proof passes for backend targets that include BIR headers.
- Focused route tests chosen by the supervisor pass.
- Include churn demonstrates reduced or clarified dependencies rather than a
  new catch-all route include.

## Step 4: Header Split Review Checkpoint

Goal: verify the split stayed narrow and decide whether the source idea can
close or needs another focused header packet.

Actions:

- Compare final moved declarations and include-site edits against the Step 1
  mapping.
- Confirm no implementation bodies, core model types, route semantics, public
  API names, namespaces, signatures, or enum values changed.
- Check for new catch-all route headers or consumers that now need many route
  headers.
- Ask the supervisor to choose broader backend validation if include churn
  reached broad backend consumers.

Completion check:

- `todo.md` records focused proof results and any supervisor-selected broader
  validation result.
- The active source idea can close only if header dependencies are reduced or
  clarified without increasing coupling.
