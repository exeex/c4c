# BIR Memory And Publication View Extraction Runbook

Status: Active
Source Idea: ideas/open/697_bir_memory_publication_view_extraction.md

## Purpose

Extract named BIR memory and publication views that separate BIR semantic
publication facts from prepared publication authority and stored route
agreement residue.

Goal: create a narrow `BirPublicationView`-style surface over existing Route 4
publication facts, migrate one Route 4 publication proof reader to named
vocabulary, and keep Route 5 cleanup deferred unless the packet owns only
named publication proof.

## Core Rule

This plan stays at the BIR memory/publication proof boundary. Route 4 and
Route 5 rows may remain private compatibility or diagnostics, but route status
and agreement rows must not become executable prepared publication authority,
source freshness, move-bundle authority, value-home authority, stack
destination authority, or MIR authority.

## Read First

- `ideas/open/697_bir_memory_publication_view_extraction.md`
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_route4_publication.cpp`
- `src/backend/bir/bir_route5_publication.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR memory access and publication route code under `src/backend/bir/`.
- Existing Route 4 block-entry and current-block publication availability
  surfaces.
- Existing Route 5 CFG-edge or current-block join-source rows only if needed
  as named publication proof, not prepared authority.
- Focused backend proof that named publication vocabulary preserves the
  selected proof reader behavior.

## Non-Goals

- Do not change executable prepared edge publication behavior.
- Do not do stack, frame, value-home, freshness, move-bundle, destination, MIR,
  or target-lowering authority work.
- Do not extract Route 7 comparison proof, Route 6 call proof, Route 8 return
  proof, or dump-policy cleanup.
- Do not rewrite expectations, unsupported markers, allowlists, timeouts,
  runtime behavior, or default harness contracts.

## Working Model

- Route 3 owns memory access identity.
- Route 4 owns current-block and block-entry publication availability.
- Route 5 mixes real CFG-edge publication facts with trailing `route5_*`
  agreement annotations.
- A named BIR memory/publication view should own the semantic proof vocabulary
  while delegating to current route builders during migration.
- Missing, unavailable, stale, mismatched, route-only, and agreement-only
  states must stay explicit and fail closed.

## Execution Rules

- Move one Route 4 publication proof reader first; do not combine unrelated
  Route 4, Route 5, memory, prealloc, and dump cleanup in one packet.
- Keep old route fields as rollback diagnostics while consumers move.
- Treat Route 5 agreement residue as diagnostic compatibility unless the
  selected packet explicitly owns named publication proof for one Route 5
  surface.
- Preserve behavior; if prepared publication behavior changes, escalate proof
  to object or runtime coverage selected by the supervisor.
- Record the first proof reader, rollback point, and proof command in
  `todo.md` before implementation.

## Steps

### Step 1: Map The First Publication View Surface

Goal: choose the smallest Route 4 block-entry or current-block publication
proof reader that can move through named BIR publication vocabulary.

Primary targets:
- `src/backend/bir/bir_route4_publication.cpp`
- `src/backend/bir/bir.hpp`
- direct Route 4 publication proof readers in backend tests or BIR/prealloc
  boundary code

Actions:
- Inspect Route 4 block-entry and current-block publication availability
  construction and lookup surfaces.
- Inspect Route 5 only enough to identify whether it must remain deferred for
  this first packet.
- Choose one low-risk proof reader or focused backend test surface.
- Record in `todo.md` the selected proof reader, old route entry point,
  intended named publication view entry point, rollback point, and supervisor
  proof command.

Completion check:
- `todo.md` names one first publication-view proof reader and a focused proof
  command, with Route 5 either explicitly deferred or scoped to one named
  publication proof surface.

### Step 2: Add The Named BIR Publication View

Goal: introduce a named memory/publication view wrapper over existing route
builders without changing behavior.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_route4_publication.cpp`
- adjacent BIR proof-view files if the repo already has a better local home

Actions:
- Add a `BirPublicationView`-style type or function for the selected Route 4
  block-entry or current-block publication proof.
- Delegate to existing Route 4 builders and lookup helpers during migration.
- Preserve current record lifetimes, statuses, diagnostics, and negative
  states.
- Keep `Route4PublicationAvailabilityIndex` private compatibility behind the
  named view.

Completion check:
- The named view compiles and returns the same publication proof result as the
  old Route 4 path for the selected surface.

### Step 3: Keep Route 5 Agreement Residue Contained

Goal: prevent Route 5 status and agreement rows from becoming prepared
authority while Route 4 publication proof moves to named vocabulary.

Primary targets:
- `src/backend/bir/bir_route5_publication.cpp`
- Route 5 dump or proof readers only if Step 1 selected them explicitly

Actions:
- If the first packet is Route 4-only, leave Route 5 unchanged and record that
  deferral in `todo.md`.
- If a Route 5 proof surface is required, expose only named publication proof
  and keep `route5_status`, `route5_agrees`, and `route5_join_source` as
  diagnostic compatibility.
- Do not use Route 5 agreement to authorize prepared publication, move-bundle,
  value-home, stack destination, or MIR behavior.

Completion check:
- Route 5 is either deferred or covered by a narrow named publication proof
  that does not change executable prepared authority.

### Step 4: Migrate One Proof Reader

Goal: move the selected proof reader from route-numbered publication lookup to
named BIR memory/publication vocabulary.

Actions:
- Replace the selected direct Route 4 or scoped Route 5 proof read with the
  named publication view API.
- Keep rollback to the old route API local and straightforward.
- Avoid stack authority, prealloc authority, target lowering, Route 7, call,
  return, and dump-policy edits.
- Add or adjust focused tests only when they prove the named publication view
  contract without weakening existing expectations.

Completion check:
- One proof reader uses named BIR publication vocabulary and behavior remains
  equivalent to the old route-numbered path.

### Step 5: Prove Publication Boundary Preservation

Goal: prove the named publication view preserves behavior and does not turn
route status or agreement residue into executable authority.

Actions:
- Run `cmake --build --preset default`.
- Run the focused backend proof command selected in Step 1.
- If the diff affects executable prepared publication behavior, run the
  stronger prepared, object, object-runtime, or runtime proof selected by the
  supervisor.
- Inspect the diff for Route 4 or Route 5 status copied into prepared
  authority, expectation rewrites, unsupported downgrades, allowlist edits,
  timeout edits, or route-dump-only proof.

Completion check:
- Build and focused proof pass.
- Route 4 and Route 5 status/agreement rows remain private compatibility or
  diagnostics.
- Prepared authority records are unchanged unless stronger proof covered an
  intentional behavior change.
