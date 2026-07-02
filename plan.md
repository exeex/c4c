# BIR Route5 Publication Body Extraction Runbook

Status: Active
Source Idea: ideas/open/526_bir_route5_publication_body_extraction.md

## Purpose

Extract route5 CFG-edge and join-source publication implementation bodies after
route3 and route4 boundaries are stable.

## Goal

Move route5 publication bodies into a focused implementation owner while
preserving public declarations and route behavior.

## Core Rule

This is a behavior-preserving body extraction only. Do not change route1,
route3, route4, route5, or route6 semantics to make the move compile or pass.

## Read First

- `ideas/open/526_bir_route5_publication_body_extraction.md`
- Current route5 declarations in `bir.hpp`
- Current route5 implementation bodies in `bir.cpp`
- `.codex/skills/c4c-clang-tools/` before mapping dependencies or moving code

## Current Targets

- Route5 CFG-edge publication bodies
- Route5 join-source publication bodies
- New `bir_route5_publication.cpp` only if a focused owner file is needed
- Private helper declarations only when required to preserve existing behavior

## Non-Goals

- Do not change route1 identity behavior.
- Do not change route3 memory records.
- Do not change route4 matching behavior.
- Do not edit route6 publication policy.
- Do not combine this with route6 extraction.
- Do not split route5 public declarations out of `bir.hpp`.
- Do not rewrite tests to weaken publication contracts.

## Working Model

Route5 is the publication owner for CFG-edge and join-source records. It may
depend on stable route1, route3, and route4 APIs, and route6 may consume route5
records. The extraction should move existing implementation bodies behind the
same declarations and preserve record ordering, selection, and visibility.

## Execution Rules

- Use AST-backed symbol queries to map route5 direct callers, callees, and type
  references before editing.
- Move bodies in small compileable steps and avoid opportunistic cleanup.
- Keep public route5 declarations in `bir.hpp`.
- Add private helper declarations only for moved existing helpers that need a
  shared internal surface.
- Treat duplicated route3 or route4 matching logic in route5 as route drift.
- Run fresh build proof after code changes.
- Run focused CFG-edge, join-source, and publication proof selected by the
  supervisor.
- Include route6 coverage if the mapped dependency graph shows route6 consumes
  route5 records in the focused subset.

## Ordered Steps

### Step 1: Map Route5 Boundaries

Goal: identify the exact route5 symbols, dependencies, and consumers before
moving any body.

Primary target: current route5 symbols in `bir.hpp` and `bir.cpp`.

Actions:

- Use `c4c-clang-tools` to list route5 declarations and definitions.
- Query direct callers and callees for route5 publication helpers.
- Map dependencies on route1, route3, and route4 APIs.
- Check whether route6 directly consumes route5 records in the target subset.
- Record any helper declarations that must remain private after the body move.

Completion check:

- The executor can name the route5 bodies to move, the dependency edges that
  must remain unchanged, and whether route6 proof is required.

### Step 2: Create Route5 Publication Owner

Goal: introduce the focused implementation owner without changing behavior.

Primary target: `bir_route5_publication.cpp`.

Actions:

- Add the new route5 owner translation unit if no suitable focused file exists.
- Move existing route5 CFG-edge and join-source publication bodies into it.
- Keep public declarations in `bir.hpp`.
- Add only required private declarations or includes for the moved bodies.
- Preserve existing namespaces, signatures, storage behavior, and record order.

Completion check:

- The project compiles far enough to prove the moved bodies are linked once and
  no public route5 declaration moved out of `bir.hpp`.

### Step 3: Repair Includes And Internal Linkage

Goal: resolve compile and linkage fallout from the body move without expanding
scope.

Primary target: includes, private helper declarations, and build registration
for the route5 owner file.

Actions:

- Add the route5 owner file to the repo-native build registration.
- Tighten includes only where the moved bodies require them.
- Keep helper visibility no broader than necessary.
- Do not duplicate route3 or route4 matching logic to avoid dependency cleanup.

Completion check:

- Fresh build proof passes for the delegated build target.

### Step 4: Focused Route5 Proof

Goal: prove that CFG-edge, join-source, and publication behavior stayed stable.

Primary target: supervisor-selected focused backend tests.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include route6 proof if Step 1 found route6 consumers in the linked subset.
- Investigate failures as behavior-preservation issues, not expectation-edit
  opportunities.

Completion check:

- Focused CFG-edge, join-source, publication, and required route6 proof passes
  without weaker expectations.

### Step 5: Acceptance Checkpoint

Goal: hand the completed body extraction back with enough proof for supervisor
review.

Primary target: final diff, `todo.md`, and canonical proof log.

Actions:

- Confirm the diff contains body movement and minimal build/include support
  only.
- Confirm route5 public declarations remain in `bir.hpp`.
- Confirm route3, route4, and route6 behavior was not changed.
- Leave proof results in the canonical executor log requested by the
  supervisor.

Completion check:

- The supervisor can review the slice against the source idea and decide
  whether broader validation or commit is ready.
