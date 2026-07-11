# Prealloc Named BIR Proof Consumer Migration Runbook

Status: Active
Source Idea: ideas/open/699_prealloc_named_bir_proof_consumer_migration.md

## Purpose

Move the first prealloc proof consumer from public route-numbered BIR proof
APIs to named BIR proof views while preserving executable prepared authority.

Goal: migrate one Route 4 block-entry attribution proof consumer through the
named publication proof view, keep the old route path as private rollback
compatibility, and prove the migrated consumer without changing prepared
execution records.

## Core Rule

This plan changes the prealloc proof-consumer boundary only. Named BIR proof
views may feed diagnostics and agreement checks, but prepared function
lookups, value homes, edge publications, move bundles, freshness, stack-source
authority, destination authority, and prepared MIR views remain the executable
authority.

## Read First

- `ideas/open/699_prealloc_named_bir_proof_consumer_migration.md`
- `docs/bir_route_index_retirement/research_digest.md`
- `docs/bir_route_index_retirement/ownership_dependencies.md`
- `docs/bir_route_index_retirement/ordered_followup_plan.md`
- `src/backend/prealloc/`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_route4_publication.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- Prealloc proof consumers that still read Route 4 publication proof rows as a
  public architecture.
- BIR-to-prealloc adapter code needed to consume the named publication proof
  view.
- Focused prepared lookup or prepared-printer proof for the first migrated
  Route 4 block-entry attribution consumer.

## Non-Goals

- Do not add new BIR view semantics.
- Do not change prepared edge publication behavior, value-home authority,
  freshness authority, move-bundle authority, stack-source authority,
  destination authority, prepared MIR behavior, object behavior, or runtime
  behavior.
- Do not migrate Route 7 comparison proof or Route 5 publication agreement rows
  until the first Route 4 prealloc consumer migration is proven.
- Do not do route dump vocabulary cleanup, expectation rewrites, unsupported
  marker changes, allowlist edits, timeout/accounting changes, or default
  harness contract changes.

## Working Model

- Named BIR publication proof views are the public proof vocabulary for the
  selected Route 4 publication surface.
- Route-numbered Route 4 APIs may remain as private compatibility and rollback
  inputs during migration.
- Prealloc proof consumers may use named BIR proof rows for diagnostics and
  agreement, but executable prepared records must remain prepared-owned.
- Missing, unavailable, ambiguous, route-only, stale, mismatched, and
  unsupported proof states must stay explicit and fail closed.

## Execution Rules

- Move one Route 4 prealloc proof consumer first; do not combine unrelated
  Route 4, Route 5, Route 7, prepared authority, MIR, target, or dump cleanup
  work in one packet.
- Record the selected consumer, old route entry point, named view entry point,
  rollback point, and supervisor proof command in `todo.md` before
  implementation.
- Keep route-numbered compatibility local to the BIR-to-prealloc boundary; do
  not copy route validation status into prepared executable records.
- If any packet changes executable prepared, MIR, object, object-runtime, or
  runtime behavior, stop and require stronger proof selected by the supervisor.
- Treat route dump text proof as insufficient for this lifecycle unless it is
  paired with the actual migrated prealloc consumer proof.

## Steps

### Step 1: Map The First Prealloc Route 4 Consumer

Goal: choose the smallest Route 4 block-entry attribution proof consumer in
prealloc or prepared lookup code that can move through the named publication
proof view.

Primary targets:
- `src/backend/prealloc/`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir_route4_publication.cpp`
- focused prepared lookup or prepared-printer tests

Actions:
- Inspect direct Route 4 publication proof reads at the BIR-to-prealloc
  boundary.
- Identify the named BIR publication proof view that can replace the selected
  route-numbered read.
- Choose one consumer with a focused proof surface and a straightforward
  rollback path to the old route API.
- Record in `todo.md` the selected consumer, old route entry point, named view
  entry point, rollback point, and focused proof command.

Completion check:
- `todo.md` names one Route 4 prealloc proof consumer and a focused proof
  command before implementation begins.

### Step 2: Confirm Prepared Authority Boundaries

Goal: ensure the selected migration does not turn route proof status into
prepared executable authority.

Primary targets:
- prepared lookup records and printers touched by the selected consumer
- adjacent value-home, publication, freshness, move-bundle, and prepared MIR
  boundary records referenced by the selected proof

Actions:
- Inspect the selected consumer's use of publication proof rows.
- Identify which data is diagnostic or agreement-only and which records are
  executable prepared authority.
- Preserve existing prepared authority records unless a later supervisor-owned
  proof escalation explicitly allows a behavior change.
- Record any authority boundary watchouts in `todo.md`.

Completion check:
- The first packet has a clear diagnostic/agreement boundary and no planned
  route-status copy into executable prepared records.

### Step 3: Add Or Reuse The BIR-To-Prealloc Named View Adapter

Goal: provide the selected prealloc consumer with named publication proof rows
without changing behavior.

Primary targets:
- `src/backend/prealloc/`
- existing named BIR publication view declarations and definitions
- the narrow BIR-to-prealloc adapter surface for the selected consumer

Actions:
- Reuse the existing named BIR publication view if it already exposes the
  required block-entry attribution proof.
- Add only the minimal adapter needed for the selected prealloc consumer when
  no suitable adapter exists.
- Delegate to existing route builders or compatibility rows behind the named
  view during migration.
- Preserve current lifetimes, statuses, diagnostics, and fail-closed negative
  states.

Completion check:
- The selected consumer can request named publication proof data through a
  BIR-to-prealloc boundary surface, with the old route API still available as
  local rollback compatibility.

### Step 4: Migrate One Prealloc Proof Consumer

Goal: replace the selected direct Route 4 proof read with named BIR
publication proof vocabulary.

Actions:
- Change only the selected consumer to use the named publication proof view or
  adapter.
- Keep rollback to the old route API local and obvious.
- Avoid Route 5 agreement migration, Route 7 comparison migration, dump
  vocabulary cleanup, prepared authority rewrites, MIR lowering, target
  materialization, and test policy changes.
- Add or adjust focused tests only when they prove the actual migrated
  consumer without weakening existing expectations.

Completion check:
- One prealloc proof consumer no longer treats a route-numbered Route 4 API as
  its public architecture, and behavior remains equivalent for the selected
  proof surface.

### Step 5: Prove The Migrated Consumer

Goal: prove the first named BIR proof consumer migration and confirm that
executable prepared authority was not changed accidentally.

Actions:
- Run `cmake --build --preset default`.
- Run the focused prepared lookup or prepared-printer proof command selected
  in Step 1.
- If executable prepared, MIR, object, object-runtime, or runtime behavior
  changed, run the stronger proof selected by the supervisor before accepting
  the packet.
- Inspect the diff for route validation status copied into prepared executable
  records, route-dump-only proof, expectation rewrites, unsupported downgrades,
  allowlist edits, timeout edits, and unrelated consumer migrations.

Completion check:
- Build and focused proof pass.
- The migrated consumer uses named BIR publication proof vocabulary.
- Route-numbered Route 4 state remains private compatibility and rollback
  input only.
- Prepared executable authority records are unchanged unless stronger proof
  covered an intentional behavior change.
