# Edge-Store Local Aggregate Publication Ordering Runbook

Status: Active
Source Idea: ideas/open/650_edge_store_local_aggregate_publication_ordering.md

## Purpose

Repair the explicit publication-ordering authority path for local values that
are published through edge-store-slot destinations or local aggregate/frame-slot
reads before RV64 consumes them.

## Goal

Classify and repair the first `src/pr68185.c` / `src/pr68321.c` owner only if
the prepared facts prove edge-store local publication ordering or local
aggregate/frame-slot publication authority. Otherwise split the row into a more
precise owner and keep this route fail-closed.

## Core Rule

RV64 may consume an edge-store or local aggregate/frame-slot publication only
when predecessor ordering, destination identity, lane or slot identity, source
freshness, and the consumer point are explicit. Do not infer publication order
from source statement order, final assembly order, diagnostic wording, block
labels, testcase identity, or scalar frame-slot facts.

## Read First

- `ideas/open/650_edge_store_local_aggregate_publication_ordering.md`
- `ideas/closed/640_mixed_local_global_publication_authority.md`
- `ideas/open/641_aggregate_global_object_materialization_policy.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`

## Current Scope

- Representative surfaces: `tests/c/external/gcc_torture/src/pr68185.c` and
  `tests/c/external/gcc_torture/src/pr68321.c`.
- Candidate family: local values published through edge-store-slot
  destinations, local aggregate lanes, or local frame-slot reads before RV64
  consumes them.
- First work is diagnostic classification, not implementation.

## Non-Goals

- Do not reopen scalar frame-slot local-memory lookup repaired by idea 640.
- Do not reopen direct global-symbol local-memory support closed by idea 631.
- Do not absorb aggregate global-object materialization owned by idea 641
  unless refreshed evidence proves that is the real first boundary.
- Do not absorb aggregate/sret/byval stack-home policy owned by idea 633.
- Do not touch move-bundle fan-in authority, ABI/runtime policy, expectations,
  unsupported markers, allowlists, timeouts, runtime-comparison policy, or
  pass/fail accounting.
- Do not special-case `src/pr68185.c`, `src/pr68321.c`, `%t38.phi`,
  `%t17.phi`, local array `g`, block names, or final emitted instruction order.

## Working Model

Idea 640 moved shared scalar frame-slot local-memory lookup forward. The
remaining `pr68185.c` and `pr68321.c` evidence is expected to involve
edge-store destination publication and local aggregate/frame-slot ownership,
not the scalar lookup family or direct global-symbol local memory.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Start by refreshing prepared-BIR, publication, and RV64 object diagnostics for
  both representative rows after the current `main` state.
- Name the predecessor edge, edge-store destination, local slot or aggregate
  lane, source value, consumer point, and available or missing ordering
  authority before selecting an implementation packet.
- Add producer or RV64 consumer support only when the selected authority is
  explicit.
- Preserve fail-closed diagnostics for ambiguous predecessor order, missing
  destination ownership, incomplete aggregate or lane identity, stale source
  values, mismatched slots or lanes, and scalar-only frame-slot facts.
- If a refreshed row belongs to aggregate global-object materialization,
  stack-home policy, or another distinct owner, record the exact boundary in
  `todo.md` and request lifecycle split or park instead of expanding this
  runbook.

## Steps

### Step 1: Refresh Representative Edge-Store Evidence

Goal: Confirm the current first owner for `src/pr68185.c` and `src/pr68321.c`.

Actions:

- Run focused prepared-BIR, semantic BIR, MIR summary or trace, and RV64
  object/diagnostic probes for both representative rows.
- Record the predecessor edge, edge-store destination, local slot or aggregate
  lane, source value, consumer point, and current missing or available authority
  in `todo.md`.
- Distinguish edge-store/local aggregate publication-ordering evidence from
  scalar frame-slot local-memory lookup, direct global-symbol local memory,
  aggregate global-object materialization, and stack-home policy.

Completion check:

- `todo.md` states whether the first owner is edge-store local publication
  ordering and identifies the next implementation or split boundary.

### Step 2: Locate The Publication-Ordering Boundary

Goal: Find the narrow producer or consumer boundary for the selected
edge-store or local aggregate publication path.

Actions:

- Trace refreshed prepared facts to the producer publication record,
  predecessor ordering evidence, destination/lane identity, and RV64
  consumer/admission path.
- Identify whether the missing owner is predecessor order, destination
  ownership, aggregate/lane identity, source freshness, or RV64 consumption of
  an already-complete authority fact.
- Identify focused backend or prepared/prealloc proof target shapes for one
  legal edge-store/local aggregate publication.
- Identify negative behavior for ambiguous predecessor order, missing
  destination ownership, aggregate-lane mismatch, stale source values, and
  scalar-only facts.

Completion check:

- `todo.md` records the owned implementation surface, focused positive test
  target shape, and fail-closed behavior to preserve.

### Step 3: Implement Or Split The Narrow Publication-Ordering Owner

Goal: Repair one proven edge-store/local aggregate publication-ordering owner,
or split if the owner is outside this idea.

Actions:

- If producer authority is missing, publish prepared/prealloc authority only
  when predecessor order, destination identity, lane or slot identity, source
  freshness, and consumer point are proven.
- If consumer authority is missing, teach the RV64 object path to consume the
  complete authority fact without guessing from source shape or stack layout.
- Add focused positive coverage for one complete-authority shape.
- Add or preserve negative coverage for ambiguous order, missing destination
  ownership, lane mismatch, stale source values, and scalar-only facts.

Completion check:

- Fresh build plus focused proof passes, or lifecycle state records the exact
  split owner and parks this route.

### Step 4: Prove Representative Integration

Goal: Show that at least one representative row advances through the
edge-store/local aggregate publication-ordering failure mode.

Actions:

- Rerun focused coverage plus the representative RV64 route for the selected
  representative row or rows.
- Capture evidence showing the selected edge-store/local aggregate publication
  consumes explicit authority, or record a distinct downstream owner.
- Do not claim completion if the row only advances by expectation changes,
  unsupported-marker changes, allowlist edits, or testcase-specific handling.

Completion check:

- `todo.md` records representative proof and any remaining downstream owner.

### Step 5: Run Broader Validation And Close Or Park

Goal: Decide whether the source idea is complete after focused and
representative proof.

Actions:

- Run the supervisor-selected broader validation for the affected backend
  bucket after focused proof is green.
- If acceptance criteria are satisfied, request plan-owner close with
  regression-guard proof.
- If a distinct downstream owner remains, record it in `todo.md` and request a
  lifecycle split or park decision.

Completion check:

- Lifecycle state either closes the source idea with passing guard proof or
  records a precise blocked or follow-up owner without broadening this idea.
