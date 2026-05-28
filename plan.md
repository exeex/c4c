# AArch64 Join Parallel-Copy Prepared Query Runbook

Status: Active
Source Idea: ideas/open/64_aarch64_join_parallel_copy_prepared_query.md

## Purpose

Move current-block join parallel-copy source relationships out of local AArch64
reconstruction and into a shared prepared join-copy query that dispatch hook
consumers can use without learning prepared move-bundle internals.

## Goal

AArch64 current-block join consumers should receive equivalent source facts
from a shared prepared query instead of rebuilding those facts from raw
prepared homes and move bundles inside dispatch-producer code.

## Core Rule

Do not preserve `build_current_block_join_parallel_copy_cache` as the semantic
authority under a renamed helper. The shared prepared move authority should own
the join-copy source relationship; AArch64 may consume the prepared fact for
target-local hook routing and emission behavior only.

## Read First

- `ideas/open/64_aarch64_join_parallel_copy_prepared_query.md`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- Dispatch hook consumers that currently read current-block join copies.
- Prepared move-bundle and value-home owners that already define move
  authority.
- Nearby branch-fusion and before-return publication consumers that may depend
  on current-block source relationships.

## Current Targets

- `build_current_block_join_parallel_copy_cache` in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.
- Current-block join-copy dispatch hook consumers.
- Prepared move-bundle and prepared value-home query inputs used by the cache.
- The shared prepared join-copy query owner that should expose equivalent
  source relationships.

## Non-Goals

- Do not change branch-fusion sequencing, before-return publication ordering,
  or target copy emission while migrating query authority.
- Do not rework same-block scalar recursion, edge fallback, or select-chain
  dependency discovery.
- Do not add one-off cache entries or testcase-shaped matching for a named join
  case.
- Do not claim broad dispatch-producer deletion without preserving current
  query semantics.
- Do not weaken supported-path expectations to hide missing prepared join-copy
  facts.

## Working Model

The current AArch64 cache reconstructs source relationships for current-block
join parallel copies by inspecting prepared homes and move bundles near the
dispatch-producer layer. That reconstruction is semantic authority leaking into
target-local code. The replacement should be a shared prepared query whose
contract matches existing prepared move authority, with AArch64 consumers
asking for structured source facts rather than rebuilding them.

The migration should be behavior-preserving for existing dispatch hook
consumers. If a relationship is ambiguous or unsupported under the shared
prepared authority, the route must fail closed or record an explicit diagnostic
gap; it must not fall back to a new local reconstruction path.

## Execution Rules

- Start by inventorying the cache inputs and every consumer of its current
  source relationships before changing code.
- Keep the semantic source-relationship query in shared prepared move or
  join-copy authority, not in AArch64 dispatch-producer code.
- Preserve consumer-visible facts for current-block joins, incoming
  expressions, prepared homes, and move bundles.
- Keep branch-fusion and before-return publication sequencing behavior out of
  scope except as proof that existing consumers still receive equivalent facts.
- Do not replace one local cache with another target-local helper that still
  reconstructs raw prepared-home and move-bundle relationships.
- Record any missing shared prepared fact in `todo.md`; do not patch it with a
  named-case shortcut.
- For code-changing steps, run at least a fresh build and the supervisor-chosen
  focused AArch64 backend subset. Escalate proof if shared prepared query
  contracts, public helper headers, or multiple dispatch hook consumers change.

## Ordered Steps

### Step 1: Inventory Current-Block Join-Copy Authority

Goal: identify every source relationship currently produced by
`build_current_block_join_parallel_copy_cache`, its raw prepared inputs, and its
dispatch hook consumers.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.

Actions:
- Inspect `build_current_block_join_parallel_copy_cache` and list each prepared
  home, move-bundle, current-block join, incoming-expression, and source-value
  fact it derives.
- Find all consumers of the cache and classify whether they need source
  identity, prepared home identity, incoming expression identity, or only
  target-local emission routing.
- Identify nearby branch-fusion and before-return publication consumers that
  could regress if source facts change.
- Identify the existing shared prepared move authority that should own each
  fact, plus any gap that must be closed before migration.
- Record the inventory and gaps in `todo.md` without implementation edits.

Completion check:
- `todo.md` names the cache-derived facts, their consumers, the proposed shared
  owner for each fact, and any blocked shared-query gaps.

### Step 2: Define The Shared Prepared Join-Copy Query

Goal: expose current-block join parallel-copy source relationships through a
shared prepared query aligned with existing prepared move authority.

Primary target:
the shared prepared move or join-copy owner identified in Step 1.

Actions:
- Add or extend a structured prepared query for current-block join-copy source
  relationships.
- Model the query around prepared move authority, not around AArch64 cache
  shape or dispatch hook convenience.
- Preserve the relationship between current-block joins, incoming expressions,
  source values, prepared homes, and move bundles.
- Define deliberate missing, ambiguous, or unsupported behavior instead of
  silently reconstructing facts locally.
- Add focused shared-level coverage or assertions where the repository already
  has prepared move or join-copy test surfaces.

Completion check:
- Shared prepared code can answer the source-relationship questions needed by
  current AArch64 consumers without requiring those consumers to inspect raw
  prepared homes and move bundles.

### Step 3: Migrate AArch64 Join-Copy Consumers To The Shared Query

Goal: replace local AArch64 cache reconstruction with calls to the shared
prepared join-copy query while preserving dispatch hook behavior.

Primary targets:
`build_current_block_join_parallel_copy_cache` and its dispatch hook consumers
in `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.

Actions:
- Replace raw prepared-home and move-bundle reconstruction with the shared
  prepared query from Step 2.
- Keep AArch64 target-local dispatch hook wiring and emission behavior local.
- Remove or shrink cache-only structures once consumers can use shared query
  results directly.
- Preserve behavior for current-block joins, incoming expression/source
  relationships, and prepared-home lookups.
- Route missing shared facts to explicit fail-closed or diagnostic behavior;
  do not reintroduce a local fallback reconstruction path.

Completion check:
- AArch64 current-block join consumers receive equivalent source facts through
  the shared prepared query, and local code no longer reconstructs those facts
  from raw prepared homes and move bundles.

### Step 4: Retire The Local Cache Authority

Goal: remove the old semantic authority from AArch64 dispatch-producer code and
leave only necessary local routing or adapters.

Primary targets:
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp` and any public helper
surface used only by the old cache.

Actions:
- Delete `build_current_block_join_parallel_copy_cache` if all consumers have
  moved, or reduce it to a thin adapter that does not own semantic
  reconstruction.
- Shrink public dispatch-producer surfaces that existed only to expose local
  cache facts.
- Ensure any remaining AArch64 helper names make target-local routing explicit
  rather than hiding old prepared move authority.
- Keep branch-fusion sequencing, before-return publication ordering, and target
  copy emission unchanged.

Completion check:
- The old cache authority is gone or demonstrably non-semantic, and no renamed
  AArch64 helper continues reconstructing join-copy relationships from raw
  prepared homes and move bundles.

### Step 5: Prove Join-Copy Query Equivalence

Goal: verify that the migration preserves current-block join-copy behavior and
does not overfit one dispatch caller.

Primary targets:
focused AArch64 backend tests and any shared prepared move or join-copy tests
affected by the new query.

Actions:
- Run the supervisor-delegated proof command after each code-changing packet.
- Cover current-block joins, incoming expression/source relationships,
  prepared homes, and move-bundle-backed source facts.
- Include nearby branch-fusion and before-return publication consumers in the
  focused proof when their input facts can come from current-block joins.
- Run `git diff --check` before returning implementation packets.
- Escalate to broader validation if shared query contracts or public helper
  headers changed, or if multiple dispatch hook consumers moved at once.

Completion check:
- Focused proof demonstrates equivalent behavior for the migrated join-copy
  consumers, and no testcase expectation is weakened or narrowed to claim
  progress.
