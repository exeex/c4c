# BIR Route6 Call Publication Body Extraction Runbook

Status: Active
Source Idea: ideas/open/528_bir_route6_call_publication_body_extraction.md

## Purpose

Extract route6 call publication implementation bodies into a focused owner
file after the earlier route body extractions have stabilized.

## Goal

Move route6 call-use, call-argument publication, call-result source, and
publication-routing bodies without changing call ABI lowering, route
dependencies, or public BIR declarations.

## Core Rule

This is a behavior-preserving body extraction only. Do not change call ABI
semantics, LIR-to-BIR call generation, route1-route5 behavior, or producer
capability to make the move compile or pass.

## Read First

- `ideas/open/528_bir_route6_call_publication_body_extraction.md`
- Current route6 declarations in `bir.hpp`
- Current route6 implementation bodies in `bir.cpp`
- Route1 through route5 public APIs and stable helper surfaces used by route6
- `.codex/skills/c4c-clang-tools/` before mapping symbols or moving code

## Current Targets

- Route6 call-use implementation bodies
- Route6 call-argument publication implementation bodies
- Route6 call-result source implementation bodies
- Route6 publication-routing implementation bodies
- New `bir_route6_call_publication.cpp` only if a focused owner file is needed
- Private helper declarations only when required to preserve existing behavior
- Existing public route6 declarations in `bir.hpp`

## Non-Goals

- Do not change call ABI lowering.
- Do not change LIR-to-BIR call generation.
- Do not implement idea 422 producer capability.
- Do not duplicate route1-route5 logic instead of using stable APIs.
- Do not move route6 public declarations out of `bir.hpp`.
- Do not move route-index facade bodies or memory provenance headers.
- Do not mix header splitting into this body-only movement.
- Do not rewrite tests to weaken call-publication, call-result, or routing
  contracts.

## Working Model

Route6 composes existing route1-route5 facts with call ABI facts. The
extraction should move cohesive route6 call publication bodies behind the same
public surface, preserving call-use classification, argument publication,
call-result source identity, and publication-routing behavior. Dependencies on
route1-route5 should stay explicit API use, not copied logic.

## Execution Rules

- Use AST-backed symbol queries to map route6 symbols, direct callers, direct
  callees, type references, and route1-route5 dependencies before editing.
- Move bodies in small compileable steps and avoid opportunistic cleanup.
- Keep public route6 declarations in `bir.hpp`.
- Add private helper declarations only for moved existing helpers that need a
  shared internal surface.
- Keep route-index facade and memory provenance surfaces out of this body move.
- Treat call ABI behavior or producer-capability changes as route drift.
- Run fresh build proof after code changes.
- Run focused call-publication, call-result, and publication-routing proof
  selected by the supervisor.
- Let the supervisor decide whether a broader `^backend_` subset is needed
  because route6 composes multiple earlier routes.

## Ordered Steps

### Step 1: Map Route6 Call Publication Boundaries

Goal: identify the exact route6 bodies, dependencies, and public/private
surfaces before moving any body.

Primary target: current route6 symbols in `bir.hpp` and `bir.cpp`.

Actions:

- Use `c4c-clang-tools` to list route6 declarations and definitions.
- Query direct callers and callees for call-use, call-argument publication,
  call-result source, and publication-routing helpers.
- Map route1-route5 APIs consumed by route6.
- Map route-index facade and memory provenance references that must stay out of
  this body move.
- Record any helper declarations that must remain private after the body move.
- Recommend focused proof commands for call-publication, call-result, and
  publication-routing behavior.

Completion check:

- The executor can name the route6 bodies to move, the dependencies that must
  stay as API calls, the helpers that must not move, and the focused proof
  needed before any body movement begins.

### Step 2: Create Route6 Call Publication Owner

Goal: introduce the focused implementation owner without changing behavior.

Primary target: `bir_route6_call_publication.cpp`.

Actions:

- Add the new route6 owner translation unit if no suitable focused file exists.
- Move existing route6 call-use, call-argument publication, call-result source,
  and publication-routing bodies into it.
- Keep public declarations in `bir.hpp`.
- Add only required private declarations or includes for the moved bodies.
- Preserve existing namespaces, signatures, route dependencies, source
  identity, publication ordering, and routing behavior.

Completion check:

- The project compiles far enough to prove the moved bodies are linked once,
  no public route6 declaration moved out of `bir.hpp`, and route-index facade
  or memory provenance surfaces stayed outside the new owner.

### Step 3: Repair Includes And Internal Linkage

Goal: resolve compile and linkage fallout from the body move without expanding
scope.

Primary target: includes, private helper declarations, and build registration
for the route6 owner file.

Actions:

- Add the route6 owner file to the repo-native build registration.
- Tighten includes only where the moved bodies require them.
- Keep helper visibility no broader than necessary.
- Do not duplicate route1-route5 logic, route-index facade logic, or memory
  provenance logic to avoid dependency cleanup.

Completion check:

- Fresh build proof passes for the delegated build target.

### Step 4: Focused Route6 Proof

Goal: prove that call publication and routing behavior stayed stable.

Primary target: supervisor-selected focused backend tests.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include call-publication proof.
- Include call-result source proof.
- Include publication-routing proof.
- Include link-time backend proof that exercises the separated route6 owner.
- Investigate failures as behavior-preservation issues, not expectation-edit
  opportunities.

Completion check:

- Focused call-publication, call-result, publication-routing, and link-time
  backend proof passes without weaker expectations.

### Step 5: Acceptance Checkpoint

Goal: hand the completed body extraction back with enough proof for supervisor
review.

Primary target: final diff, `todo.md`, and canonical proof log.

Actions:

- Confirm the diff contains route6 body movement and minimal build/include
  support only.
- Confirm route6 public declarations remain in `bir.hpp`.
- Confirm route-index facade and memory provenance surfaces did not move.
- Confirm call ABI lowering, LIR-to-BIR call generation, and route1-route5
  behavior did not change.
- Leave proof results in the canonical executor log requested by the
  supervisor.
- Note whether the supervisor should escalate to a broader `^backend_` subset.

Completion check:

- The supervisor can review the slice against the source idea and decide
  whether broader validation or commit is ready.
