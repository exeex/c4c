# BIR CFG Edge-Publication Source Identity Completion Runbook

Status: Active
Source Idea: ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md
Activated after parking: ideas/open/717_prepared_mir_join_source_identity_completion.md

## Purpose

Replace the circular prepared/prepared agreement path with an independent typed
BIR CFG edge-publication source identity resolver.

## Goal

Resolve predecessor, successor, destination, source, and producer or memory
identity from the supplied BIR CFG request, then compare that authority with
prepared facts while preserving typed fail-closed states.

## Core Rule

The BIR CFG side must establish identity from BIR CFG authority. Never claim
BIR/prepared agreement by adapting prepared facts twice.

## Read First

- `ideas/open/719_bir_cfg_edge_publication_source_identity_completion.md`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/mir/query.h`
- `src/backend/mir/query.cpp`

## Current Scope

- `BirCfgEdgePublicationSourceRequest` consumption and legacy query adaptation.
- Typed CFG edge, destination, producer-instruction, and producer-memory
  identity.
- Focused common query proof across supported and negative source shapes.

## Non-Goals

- Do not change prepared-MIR current-block join-source preparation.
- Do not change prepared-call or block-entry publication behavior.
- Do not change target materializers, allocation, or move scheduling.
- Do not weaken unsupported, missing, stale, duplicate, or mismatched states.

## Execution Rules

- Establish the first discarded request fact before changing behavior.
- Generalize across load, cast, binary, select, and memory-producing shapes.
- Keep prepared and BIR authority independent until the final typed comparison.
- Use only supervisor-delegated build, focused, and broader proof commands.

## Ordered Steps

### Step 1: Localize request loss and independent BIR authority

Goal: identify the earliest point where the supplied CFG request is ignored and
the exact BIR facts capable of resolving each supported identity field.

Actions:

- Trace the request-taking test helper through all legacy query overloads.
- Map predecessor, successor, destination, source, producer instruction, and
  producer memory identity to their BIR CFG owners.
- Classify load, cast, binary, select, missing-destination, unavailable-source,
  stale, duplicate, and mismatch paths before selecting a repair seam.

Completion check:

- `todo.md` records the first discarded fact, owning helper, typed authority
  map, and a general repair rule without an implementation change.

### Step 2: Implement independent typed CFG identity resolution

Goal: consume the request and resolve exact BIR CFG source identity without
prepared-derived authority.

Actions:

- Repair the smallest common query boundary at the first incorrect fact.
- Validate exact predecessor/successor edge and destination identity.
- Resolve exact producer instruction or memory identity for every supported
  source shape.
- Preserve typed missing, unavailable, stale, duplicate, and mismatch results.

Completion check:

- Supported rows expose independently resolved BIR CFG identity and all negative
  rows fail closed without row-order, name-only, or prepared-fact selection.

### Step 3: Prove independent agreement across the source family

Goal: lock the non-circular contract beyond the first load-local failure.

Actions:

- Add focused positive assertions for load, cast, binary, select, and relevant
  memory producer shapes.
- Add wrong-edge, missing-destination, unavailable-source, stale, duplicate,
  producer-mismatch, and memory-mismatch proof.
- Run the supervisor-delegated build and focused query subset.

Completion check:

- Focused tests prove that the request changes or rejects the BIR result when
  its typed authority changes, without expectation downgrade.

### Step 4: Run acceptance proof and route prerequisites

Goal: establish closure-quality evidence and return to parked prerequisite
acceptance.

Actions:

- Audit the diff for fixture, name, row-order, route, and prepared-oracle
  shortcuts.
- Run the supervisor-selected broader backend before/after comparison.
- Record whether idea 717 can resume its remaining acceptance checks before
  routing idea 716 or idea 718.

Completion check:

- Focused and broader proof are green, reviewer reject signals are absent, and
  lifecycle routing can return to idea 717 acceptance.
