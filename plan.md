# Phase E Route 3 Memory Semantic View Consumer Migration

Status: Active
Source Idea: ideas/open/186_phase_e_route3_memory_semantic_view_consumer_migration.md

## Purpose

Migrate one AArch64 memory/source identity reader to a Route 3 semantic memory
view while keeping prepared memory/access helpers public as fallback and oracle
surfaces.

## Goal

Switch exactly one selected consumer path to Route 3 facts for semantic memory
access identity, same-block global-load identity, or load-local stored-value
source relationships.

## Core Rule

Route 3 may expose only semantic memory/source facts. Do not move address base
kind, offsets, stack/frame objects, TLS/global relocation data, pointer
materialization, base-plus-offset legality, volatile/address-space lowering,
final memory operand records, or broad prepared aggregate ownership into the
Route 3 view.

## Read First

- `ideas/open/186_phase_e_route3_memory_semantic_view_consumer_migration.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- Relevant AArch64 consumer files discovered from the selected path, such as
  `src/backend/mir/aarch64/codegen/memory.cpp`,
  `src/backend/mir/aarch64/codegen/globals.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`, or related load-local,
  global-load, and store-source publication paths.

## Current Targets / Scope

- One AArch64 memory, globals, or load-local source recovery path that needs
  semantic access identity rather than target address formation.
- A Route 3 view or narrow adapter returning access identity, same-block
  global-load identity, or load-local stored-value source relationships.
- Prepared memory/access, global-load, same-block global-load, and load-local
  source helpers retained as public fallbacks and equivalence oracles.

## Non-Goals

- Do not delete or hide prepared memory/access helpers.
- Do not migrate multiple consumer families in this plan.
- Do not rewrite expectation contracts to claim progress.
- Do not add named-case shortcuts, broad BIR rescans, predecessor rescans, or
  name matching to recover missing Route 3 facts.
- Do not copy `PreparedAddress`, `PreparedMemoryAccess`, or
  `PreparedFunctionLookups` wholesale into BIR or under a BIR-owned name.

## Working Model

- Route 3 records and indexes already own target-neutral semantic memory/source
  identity facts.
- Prepared helpers remain necessary for residual consumers and as equivalence
  oracles during migration.
- The migrated consumer should read Route 3 first when a valid semantic record
  is available and preserve prepared fallback behavior where the selected path
  still requires fail-closed or compatibility behavior.
- Any target policy needed to form addresses or emit machine records stays in
  prepared/AArch64 code and must not be threaded into the Route 3 view.

## Execution Rules

- Keep each implementation packet bounded to one consumer path or one test
  equivalence surface.
- Before editing, identify the prepared helper call being replaced and the
  corresponding Route 3 record/index query.
- Preserve prepared oracle tests and add route/prepared agreement coverage
  instead of weakening expectations.
- Use build proof for every code-changing packet, then the supervisor-selected
  narrow test subset. Escalate to broader backend validation if public helper
  signatures, traversal context wiring, address materialization policy, or
  shared lookup APIs change.
- Treat testcase-shaped matching, unsupported downgrades, and expectation-only
  changes as route drift.

## Ordered Steps

### Step 1: Select One Route 3 Consumer

Goal: Identify the narrowest AArch64 consumer path that can read Route 3
memory/source facts without importing target address policy into BIR.

Primary target:
- One prepared memory/access, global-load, same-block global-load, or
  load-local source reader in AArch64 memory, globals, call, or store-source
  lowering.

Actions:
- Inspect current prepared helper users for memory access identity,
  global-load identity, same-block global-load identity, and load-local
  stored-value source facts.
- Choose one consumer path with nearby Route 3 oracle coverage and a clear
  fallback story.
- Record the chosen prepared helper, Route 3 query surface, and proof subset in
  `todo.md` before implementation begins.
- Do not choose a path that requires moving address formation, frame slots,
  relocation/TLS policy, stack layout, base-plus-offset legality, volatile or
  address-space lowering, memory operand formation, or final record spelling
  into Route 3.

Completion check:
- `todo.md` names one selected consumer path, its prepared fallback/oracle, and
  the narrow proof command the supervisor should delegate to an executor.

### Step 2: Expose Or Reuse A Narrow Route 3 View

Goal: Make the selected consumer able to read Route 3 facts through a narrow,
fail-closed semantic surface.

Primary target:
- Existing Route 3 index/query helpers or a small validated adapter around
  them.

Actions:
- Prefer existing Route 3 query APIs and indexes before adding a new wrapper.
- If a wrapper is needed, key it by the selected consumer's existing semantic
  handle and return only access identity, same-block global-load identity, or
  load-local stored-value source relationships.
- Preserve prepared helper visibility and fallback behavior.
- Add or update focused helper tests for present route data, absent route data,
  invalid or mismatched references, and prepared agreement as needed for the
  selected surface.

Completion check:
- The selected consumer has a Route 3-readable semantic view, and helper-level
  tests prove agreement with prepared oracle behavior without weakening
  prepared tests.

### Step 3: Migrate The Selected Consumer

Goal: Switch exactly one selected AArch64 consumer to use Route 3 facts where
available.

Primary target:
- The selected consumer file from Step 1.

Actions:
- Replace the selected semantic prepared read with the Route 3 view where it
  answers the needed fact.
- Keep prepared fallback for absent Route 3 records or compatibility paths that
  still require it.
- Leave target-owned addressing, frame/layout, relocation/TLS, memory operand,
  volatile/address-space lowering, store-publication policy, and final record
  decisions in the existing AArch64/prepared code.
- Do not touch unrelated consumer families.

Completion check:
- The selected consumer reads Route 3 semantic facts on the intended path,
  prepared fallback still works, and no unrelated prepared API contraction is
  claimed.

### Step 4: Prove Route/Prepared Equivalence

Goal: Show that the migrated consumer preserves behavior across the Route 3
cases required by the source idea.

Primary target:
- Existing prepared lookup helper tests, Route 3 oracle tests, and the narrow
  AArch64 backend test subset that exercises the selected consumer.

Actions:
- Add or adjust tests so the migrated consumer is covered for global-load and
  same-block global-load cases when relevant to the selected path.
- Cover local-stored source, absent-route, and non-semantic
  target-address-only cases where they apply.
- Keep address-materialization tests active to prove target policy remains
  prepared-owned.
- Keep prepared oracle tests active and unweakened.
- Run the build plus supervisor-selected narrow proof command and store the
  executor proof in `test_after.log`.

Completion check:
- Build proof and narrow route/prepared equivalence proof are green, and the
  result does not rely on expectation downgrades or selected-testcase matching.

### Step 5: Handoff For Review Or Next Lifecycle Decision

Goal: Leave the lifecycle state clear for supervisor review after the selected
consumer migration is proven.

Actions:
- Update `todo.md` with the completed packet summary, proof command, and any
  remaining Route 3 residual consumers discovered during execution.
- If the runbook is exhausted but the source idea still has residual scope,
  report that explicitly instead of closing the idea automatically.
- If implementation changed shared Route 3 or prepared helper APIs, recommend
  broader backend validation to the supervisor.

Completion check:
- `todo.md` states whether idea 186 is complete, blocked, or needs a follow-up
  route/lifecycle decision, with proof results attached.
