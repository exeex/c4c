# BIR Call And Return Route View Extraction Runbook

Status: Active
Source Idea: ideas/open/698_bir_call_return_route_view_extraction.md

## Purpose

Extract named BIR call-boundary and return-chain views for Route 6 and Route 8
facts without folding target ABI lowering or return materialization policy into
the BIR view layer.

Goal: add a narrow named call-boundary view or proof adapter over existing
Route 6 call argument/result source facts, migrate one first proof reader, and
decide whether Route 8 return-chain facts are public BIR semantics or
target-specific compatibility.

## Core Rule

This plan stays at the BIR call-boundary and return-chain semantic view layer.
Route 6 and Route 8 facts may remain private compatibility internals, but they
must not become target ABI policy, prepared destination authority, frame layout
authority, value-home authority, freshness authority, move-bundle authority,
or MIR lowering authority.

## Read First

- `ideas/open/698_bir_call_return_route_view_extraction.md`
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_route6_call_publication.cpp`
- `src/backend/bir/bir_route8.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- BIR call-boundary route code under `src/backend/bir/`.
- Existing Route 6 call argument/result source reconstruction surfaces.
- Route 8 return-chain facts only enough to decide whether they belong in a
  public named BIR view or should remain target-specific compatibility.
- Focused backend proof that the named call-boundary view preserves current
  call argument/result source reconstruction.

## Non-Goals

- Do not change target ABI lowering, call lowering, return lowering, MIR
  lowering, object behavior, runtime behavior, prepared destination authority,
  frame layout authority, value-home authority, freshness authority, or
  move-bundle authority.
- Do not repair stack destination fan-in.
- Do not extract publication, memory, producer, or dump-policy views.
- Do not rewrite call/return expectations, unsupported markers, allowlists,
  timeouts, default harness contracts, or route dumps as proof of migration.

## Working Model

- Route 6 reconstructs call argument/result source facts at BIR call
  boundaries.
- Route 8 records return-chain value identity, but may be target-specific
  compatibility rather than a public BIR semantic view.
- A named BIR call-boundary view should own call-boundary proof vocabulary
  while delegating to existing route builders during migration.
- Missing, unavailable, stale, mismatched, wrong-block, wrong-role, and
  route-only evidence must stay explicit and fail closed.

## Execution Rules

- Move one Route 6 diagnostic or proof reader first; do not combine unrelated
  Route 6, Route 8, ABI, target lowering, and stack work.
- Preserve old route data as rollback compatibility while consumers move.
- Decide Route 8's public-view status from evidence; do not force it into the
  first Route 6 migration if the return-chain facts are target-specific.
- If any packet changes executable call or return behavior, stop and require
  target MIR, object, object-runtime, or runtime proof selected by the
  supervisor.
- Record the first proof reader, rollback point, Route 8 decision, and proof
  command in `todo.md`.

## Steps

### Step 1: Map The First Call-Boundary Surface

Goal: choose the smallest Route 6 call argument/result source proof reader
that can move through named BIR call-boundary vocabulary.

Primary targets:
- `src/backend/bir/bir_route6_call_publication.cpp`
- `src/backend/bir/bir.hpp`
- direct Route 6 proof readers in backend tests or BIR/prealloc boundary code

Actions:
- Inspect Route 6 call argument/result source index construction and lookup
  surfaces.
- Inspect focused backend tests that already prove call argument/result source
  reconstruction.
- Choose one low-risk proof reader or focused backend test surface.
- Record in `todo.md` the selected proof reader, old route entry point,
  intended named call-boundary view entry point, rollback point, and
  supervisor proof command.

Completion check:
- `todo.md` names one first Route 6 call-boundary proof reader and a focused
  proof command.

### Step 2: Decide Route 8 View Boundary

Goal: determine whether Route 8 return-chain facts are public BIR semantics or
target-specific compatibility for this lifecycle step.

Primary targets:
- `src/backend/bir/bir_route8.cpp`
- Route 8 declarations in `src/backend/bir/bir.hpp`
- current Route 8 proof readers, if any

Actions:
- Inspect Route 8 return-chain fact shape and current consumers.
- Decide whether Route 8 should be included in a named BIR return-chain view
  now, explicitly deferred, or left as target-specific compatibility.
- Record the decision in `todo.md`; do not expand implementation beyond the
  selected Route 6 proof reader unless Route 8 is required by that proof.

Completion check:
- `todo.md` records a concrete Route 8 boundary decision with no silent
  return-chain scope expansion.

### Step 3: Add The Named Call-Boundary View

Goal: introduce a named Route 6 call-boundary view or proof adapter over
existing route builders without changing behavior.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_route6_call_publication.cpp`
- adjacent BIR proof-view files if the repo already has a better local home

Actions:
- Add a `BirCallBoundaryView`-style type or function for the selected Route 6
  call argument/result source proof.
- Delegate to existing Route 6 builders and lookup helpers during migration.
- Preserve current record lifetimes, statuses, diagnostics, and negative
  states.
- Keep `Route6CallUseSourceIndex` private compatibility behind the named view.

Completion check:
- The named view compiles and returns the same call-boundary proof result as
  the old Route 6 path for the selected surface.

### Step 4: Migrate One Proof Reader

Goal: move the selected proof reader from route-numbered Route 6 lookup to
named BIR call-boundary vocabulary.

Actions:
- Replace the selected direct Route 6 proof read with the named
  call-boundary view API.
- Keep rollback to the old route API local and straightforward.
- Avoid ABI lowering, target lowering, stack authority, publication/memory
  cleanup, Route 8 expansion, and dump-policy edits unless Step 2 explicitly
  proves they are required.
- Add or adjust focused tests only when they prove the named call-boundary
  view contract without weakening existing expectations.

Completion check:
- One proof reader uses named BIR call-boundary vocabulary and behavior
  remains equivalent to the old route-numbered path.

### Step 5: Prove Call-Boundary Preservation

Goal: prove the named call-boundary view preserves call argument/result source
reconstruction and does not turn route facts into executable authority.

Actions:
- Run `cmake --build --preset default`.
- Run the focused backend proof command selected in Step 1.
- If the diff affects executable call or return behavior, run the stronger
  target MIR, object, object-runtime, or runtime proof selected by the
  supervisor.
- Inspect the diff for Route 6 or Route 8 facts copied into ABI, prepared,
  MIR, value-home, frame-layout, freshness, move-bundle, or destination
  authority.
- Reject expectation rewrites, unsupported downgrades, allowlist edits,
  timeout edits, or route-dump-only proof.

Completion check:
- Build and focused proof pass.
- Route 6 and Route 8 remain private compatibility unless a named view owns
  the semantic contract.
- Missing call or return evidence remains explicit and fail-closed.
