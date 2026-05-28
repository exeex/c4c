# AArch64 Shared Edge Dependency Authority Runbook

Status: Active
Source Idea: ideas/open/62_aarch64_shared_edge_dependency_authority.md

## Purpose

Move predecessor/successor edge source identity out of AArch64 edge-copy
fallback discovery and into shared prepared edge dependency or prepared
publication-query authority, while keeping target edge-copy emission local.

## Goal

AArch64 edge-copy emission should consume shared/prepared edge source facts
instead of recovering missing publication producers through predecessor-depth
scans.

## Core Rule

Do not preserve AArch64 predecessor-depth scans as the semantic authority for
edge-copy source identity. AArch64 may own target-local scratch, hazard,
clobber, va-list carrier, and instruction-ordering behavior only after the
shared/prepared edge fact is known.

## Read First

- `ideas/open/62_aarch64_shared_edge_dependency_authority.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- AArch64 prepared edge-publication consumers used by the edge copy route.
- Shared prepared edge dependency, publication, or move-authority owners that
  can expose successor, edge, and predecessor source facts.

## Current Targets

- `find_edge_named_producer` in
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.
- Null-publication callers in the AArch64 edge-copy route.
- Prepared edge-publication consumers that should receive authoritative source
  facts.
- Shared edge dependency or prepared publication-query APIs that should expose
  equivalent successor, edge, and predecessor source relationships.

## Non-Goals

- Do not move clobber-sensitive copy ordering, scratch-register selection,
  va-list carrier emission, target instruction spelling, or machine
  instruction ordering to shared code.
- Do not rework non-edge select-chain discovery.
- Do not rewrite branch-fusion or before-return publication sequencing.
- Do not delete fallback code until its callers are proven covered by
  prepared edge facts or deliberate fail-closed diagnostics.
- Do not weaken supported-path expectations or mark edge-copy cases
  unsupported to hide missing prepared facts.

## Working Model

The current edge-copy route can recover source producers from AArch64-local
predecessor-depth scans when prepared publication facts are missing or null.
That recovery path duplicates semantic authority that should live in shared
prepared edge dependency or publication-query state. This runbook migrates the
source identity question to shared/prepared ownership and leaves AArch64 with
the target emission details needed to spell correct machine copies.

Missing prepared facts must fail closed or produce a durable diagnostic. They
must not silently route through a renamed AArch64 scan.

## Execution Rules

- Start with an inventory of every null-publication caller and fallback
  producer-discovery path before changing behavior.
- Separate source identity from target edge-copy emission in every edited
  helper.
- Prefer one shared/prepared edge query contract that covers predecessor,
  successor, and edge source facts for all current AArch64 callers.
- Do not add named-case filters, testcase-shaped producer matching, or raw BIR
  scans under AArch64 edge-copy dispatch.
- Keep scratch selection, copy ordering, clobber handling, va-list carrier
  emission, and target instruction spelling inside AArch64.
- Record any missing shared fact gap in `todo.md`; do not fill it with a new
  AArch64 fallback.
- For code-changing steps, run at least a fresh build and the focused AArch64
  backend subset chosen by the supervisor. Escalate proof if shared query
  contracts or public helper headers change.

## Ordered Steps

### Step 1: Inventory Edge Publication Fallbacks

Goal: classify all AArch64 edge-copy source-discovery paths by current
authority, required edge fact, and target emission responsibility.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.

Actions:
- Find `find_edge_named_producer` callers and every null-publication recovery
  path in the edge-copy route.
- Identify which paths need predecessor, successor, edge, or publication source
  identity.
- Separate helpers that only perform target edge emission from helpers that
  rediscover semantic source identity.
- Identify existing shared prepared edge dependency or publication-query APIs
  that already expose equivalent facts.
- Record missing shared/prepared facts that must exist before local fallback
  discovery can be removed.
- Identify negative cases where missing facts should fail closed or diagnose.

Completion check:
- `todo.md` records the fallback inventory, the shared owner candidate for
  each source fact, and any blocked fact gaps without implementation edits.

### Step 2: Define Shared Edge Source Facts

Goal: expose the edge source identity AArch64 needs through shared prepared
edge dependency or publication-query authority.

Primary target:
the shared prepared edge dependency, publication-query, or move-authority owner
identified in Step 1.

Actions:
- Add or extend structured query APIs for predecessor, successor, edge, and
  publication source facts required by AArch64 edge-copy emission.
- Keep query results descriptive enough for AArch64 to emit copies without
  re-scanning predecessors for source identity.
- Preserve existing prepared authority semantics for missing, ambiguous, or
  unsupported edge facts.
- Add focused shared-level coverage or assertions where the repository has an
  established test surface for prepared edge facts.

Completion check:
- Shared/prepared APIs can answer the in-scope edge source questions without
  AArch64 predecessor-depth scans, and missing facts have deliberate
  fail-closed or diagnostic behavior.

### Step 3: Migrate Prepared Edge-Publication Consumers

Goal: make AArch64 edge-copy consumers use shared/prepared edge source facts
before fallback removal.

Primary target:
prepared edge-publication consumers in
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`.

Actions:
- Replace local fallback source identity decisions with the shared/prepared
  edge query API from Step 2.
- Keep target-local scratch, clobber, and copy-ordering checks at the AArch64
  emission layer.
- Preserve behavior for covered prepared edge sources while routing missing
  facts to explicit fail-closed or diagnostic behavior.
- Avoid widening the call surface so non-edge select-chain or branch-fusion
  logic starts depending on edge-copy internals.

Completion check:
- Prepared edge-publication consumers receive equivalent source facts from
  shared/prepared authority, and no migrated caller depends on predecessor
  scans for semantic producer identity.

### Step 4: Remove Or Fail-Close Null-Publication Recovery

Goal: retire the null-publication fallback as semantic recovery once its
callers are covered by prepared edge facts.

Primary target:
`find_edge_named_producer` and null-publication fallback callers.

Actions:
- Remove fallback producer discovery where shared/prepared edge facts now cover
  the caller.
- Replace uncovered missing-fact paths with deliberate fail-closed behavior or
  durable diagnostics, not local source recovery.
- Delete or narrow helper code whose only role was predecessor-depth semantic
  producer discovery.
- Confirm va-list carrier, clobber-sensitive copy ordering, scratch choice, and
  instruction sequencing remain unchanged.

Completion check:
- AArch64 edge-copy code no longer treats null publication as a cue to
  rediscover producers locally, and any missing prepared fact is explicit and
  reviewable.

### Step 5: Prove Edge Coverage And Target-Local Preservation

Goal: validate shared edge source authority without overfitting one edge-copy
testcase.

Primary target:
focused AArch64 backend tests and any shared prepared/query tests touched by
the migration.

Actions:
- Run a fresh build or compile proof selected by the supervisor.
- Run focused proof for prepared edge sources, missing publication facts,
  va-list carrier emission, predecessor/successor edge inputs, and
  clobber-sensitive copy ordering.
- Include nearby negative coverage for missing shared/prepared edge facts where
  the repo has a suitable test surface.
- Inspect the diff for new AArch64 predecessor scans, renamed fallback helpers,
  named-case shortcuts, weakened expectations, or unsupported downgrades.
- Ask the supervisor to escalate to broader backend or regression-guard proof
  if shared query contracts, public headers, or multiple backend buckets were
  changed.

Completion check:
- Build and focused proof are green, `todo.md` records exact commands and log
  names, and the diff contains no durable AArch64 edge source fallback that
  replaces the old local discovery under a new helper name.
