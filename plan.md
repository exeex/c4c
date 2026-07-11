# BIR Route Facade Named Compatibility Adapters Runbook

Status: Active
Source Idea: ideas/open/695_bir_route_facade_named_compatibility_adapters.md

## Purpose

Contract the narrow `bir_route_index` facade by adding ownership-named BIR
compatibility adapters for Route 4 publication validation and Route 7
comparison validation, then migrate one low-risk caller without changing
behavior or prepared authority.

Goal: make the first facade consumer depend on named publication or comparison
proof vocabulary instead of treating `RouteIndexReferenceFacade` as a public
architecture boundary.

## Core Rule

Keep this packet behavior-preserving. Route-numbered records may remain private
compatibility inputs, but they must not become prepared publication,
freshness, value-home, stack destination, frame-layout, branch stack-load,
move-bundle, MIR, or executable authority.

## Read First

- `ideas/open/695_bir_route_facade_named_compatibility_adapters.md`
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
- `src/backend/bir/bir_route_index.hpp`
- `src/backend/bir/bir_route_facade.cpp`
- `src/backend/bir/bir_route4_publication.cpp`
- `src/backend/bir/bir_route7_comparison.cpp`

## Current Targets

- BIR compatibility and proof-adapter code under `src/backend/bir/`.
- The first low-risk Route 4 block-entry attribution caller or Route 7
  comparison diagnostic caller that can move through a named adapter.
- Focused backend proof only when it proves the named adapter contract without
  expectation or harness-policy changes.

## Non-Goals

- Do not rebuild Route 1 through Route 8 semantics.
- Do not change prealloc executable authority or target lowering behavior.
- Do not perform Route 5 publication cleanup.
- Do not repair stack destination authority.
- Do not retire dump vocabulary or rewrite expectations in this plan.
- Do not use unsupported-marker, allowlist, timeout, accounting, or runtime
  contract changes as proof.

## Working Model

- `RouteIndexReferenceFacade` is a compatibility facade over Route 4
  publication validation and Route 7 comparison validation.
- New named adapters should describe the proof ownership, such as publication
  reference validation or comparison reference validation, while delegating to
  the current route builders during migration.
- Existing route-numbered types may remain private rollback inputs until
  callers are moved.
- Missing or unavailable adapter states should remain explicit and fail closed.

## Execution Rules

- Move one consumer surface first; do not combine unrelated Route 4 and Route 7
  migrations unless the first proof requires both.
- Preserve current record lifetimes, statuses, diagnostics, and behavior.
- Keep public API naming tied to BIR proof ownership, not route numbers.
- If an implementation changes executable behavior, stop and escalate proof to
  prepared MIR, object, object-runtime, or runtime coverage before acceptance.
- Reject helper renames or wrapper-only churn if the migrated caller still
  depends on the old public facade as architecture.

## Steps

### Step 1: Map The First Facade Consumer

Goal: identify the smallest caller that can move from direct
`RouteIndexReferenceFacade` validation to a named publication or comparison
proof adapter.

Primary targets:
- `src/backend/bir/bir_route_index.hpp`
- `src/backend/bir/bir_route_facade.cpp`
- direct callers of `route_index_validate_*`

Actions:
- Inspect all current direct `RouteIndexReferenceFacade` construction and
  validation callers.
- Choose either one Route 4 block-entry/current-block attribution caller or one
  Route 7 comparison diagnostic caller as the first migration.
- Record the chosen proof surface and rollback point in `todo.md`, not in the
  source idea.
- Confirm that the selected caller can be migrated without expectation,
  unsupported-marker, allowlist, timeout, or executable behavior changes.

Completion check:
- `todo.md` names one first consumer, its current facade entry point, the
  intended named adapter, and the focused proof command the supervisor should
  delegate.

### Step 2: Add Named Compatibility Adapter Surface

Goal: introduce ownership-named adapter entry points over the existing Route 4
or Route 7 facade path.

Primary targets:
- `src/backend/bir/bir_route_index.hpp`
- `src/backend/bir/bir_route_facade.cpp`
- adjacent BIR proof-view headers or implementations if the repo already has
  a better local naming pattern

Actions:
- Add a named adapter type or function for the selected proof family.
- Keep `RouteIndexReferenceFacade`, `Route4IndexReferenceValidation`, and
  `Route7IndexReferenceValidation` as private compatibility inputs or rollback
  records as needed.
- Preserve existing missing-index, missing-record, stale-reference, and
  mismatch statuses.
- Do not expose `RouteIndexRoute`, `RouteIndexRecordReference`, or route
  validation status as a new durable public architecture boundary.

Completion check:
- The named adapter compiles and returns the same validation outcome as the old
  facade for the selected surface.

### Step 3: Migrate One First Consumer

Goal: move the selected low-risk caller through the named adapter while leaving
route-numbered compatibility available behind it.

Primary targets:
- the caller selected in Step 1
- focused BIR/backend proof files only if needed for the adapter contract

Actions:
- Replace direct public-facade use at the selected call site with the named
  publication or comparison adapter.
- Keep rollback to the old facade local and straightforward.
- Avoid broad route-builder rewrites, Route 5 changes, prepared authority
  changes, or target-lowering changes.
- Add or adjust focused tests only when they prove the migrated adapter
  contract without weakening an existing expectation.

Completion check:
- The first migrated consumer no longer treats `RouteIndexReferenceFacade` as
  its public proof boundary, and behavior remains equivalent.

### Step 4: Prove Behavior Preservation And Private Compatibility

Goal: produce fresh proof that the migrated surface is behavior-preserving and
that route-numbered records remain compatibility details.

Actions:
- Run `cmake --build --preset default`.
- Run the focused backend or BIR test subset selected in Step 1.
- If the diff unexpectedly affects executable behavior, run the stronger
  prepared MIR, object, object-runtime, or runtime proof selected by the
  supervisor.
- Inspect the diff for public expansion of route-numbered APIs or expectation
  rewrites.

Completion check:
- Build and focused proof pass.
- No route-numbered API is newly exposed as public architecture.
- No expectation rewrite, unsupported downgrade, allowlist edit, timeout edit,
  accounting edit, or weaker runtime check is used as progress.
