# Phase E Route 2 Select-Chain View Consumer Migration

Status: Active
Source Idea: ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md

## Purpose

Migrate one AArch64 select-chain or direct-global materialization reader to a
Route 2 semantic view while keeping prepared select-chain/direct-global helpers
public as fallback and oracle surfaces.

## Goal

Switch exactly one selected consumer path to Route 2 facts for select-root
identity, root instruction index, scalar eligibility, and direct-global
dependency presence.

## Core Rule

Route 2 may expose only semantic select-chain/direct-global facts. Do not move
target materialization sequence choice, storage/home selection, memory operand
formation, call-publication policy, final record spelling, or broad prepared
aggregate ownership into the Route 2 view.

## Read First

- `ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `src/backend/prealloc/select_chain_lookups.hpp`
- Relevant AArch64 consumer files discovered from the selected path, such as
  `src/backend/mir/aarch64/codegen/select_materialization.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `src/backend/mir/aarch64/codegen/memory.cpp`, or
  `src/backend/mir/aarch64/codegen/alu.cpp`.

## Current Targets / Scope

- One AArch64 select materialization, direct-global dependency, scalar
  publication, call, memory, or ALU subpath that needs select-root facts.
- A Route 2 view or narrow adapter returning select-root identity, root
  instruction index, scalar eligibility, and direct-global dependency presence.
- Prepared select-chain materialization and direct-global dependency helpers
  retained as public fallbacks and equivalence oracles.

## Non-Goals

- Do not delete or hide prepared select-chain/direct-global helpers.
- Do not migrate multiple consumer families in this plan.
- Do not rewrite expectation contracts to claim progress.
- Do not add named-case shortcuts, broad BIR rescans, predecessor rescans, or
  name matching to recover missing Route 2 facts.
- Do not contract `PreparedFunctionLookups` or copy it under a BIR-owned name.

## Working Model

- Route 2 records and indexes already own target-neutral select-chain facts.
- Prepared helpers remain necessary for residual consumers and as equivalence
  oracles during migration.
- The migrated consumer should read Route 2 first when a valid record is
  available and preserve prepared fallback behavior where the selected path
  still requires fail-closed or compatibility behavior.
- Any target policy needed to emit machine records stays in prepared/AArch64
  code and must not be threaded into the Route 2 view.

## Execution Rules

- Keep each implementation packet bounded to one consumer path or one test
  equivalence surface.
- Before editing, identify the prepared helper call being replaced and the
  corresponding Route 2 record/index query.
- Preserve prepared oracle tests and add route/prepared agreement coverage
  instead of weakening expectations.
- Use build proof for every code-changing packet, then the supervisor-selected
  narrow test subset. Escalate to broader backend validation if public helper
  signatures, traversal context wiring, or shared lookup APIs change.
- Treat testcase-shaped matching, unsupported downgrades, and expectation-only
  changes as route drift.

## Ordered Steps

### Step 1: Select One Route 2 Consumer

Goal: Identify the narrowest AArch64 consumer path that can read Route 2
select-chain/direct-global facts without importing target policy into BIR.

Primary target:
- One prepared select-chain/direct-global reader in AArch64 materialization,
  publication, call, memory, or ALU lowering.

Actions:
- Inspect current prepared helper users for select-chain materialization and
  direct-global dependency facts.
- Choose one consumer path with nearby Route 2 oracle coverage and a clear
  fallback story.
- Record the chosen prepared helper, Route 2 query surface, and proof subset in
  `todo.md` before implementation begins.
- Do not choose a path that requires moving homes, registers, memory operand
  formation, call ABI, publication routing policy, or final record spelling
  into Route 2.

Completion check:
- `todo.md` names one selected consumer path, its prepared fallback/oracle, and
  the narrow proof command the supervisor should delegate to an executor.

### Step 2: Expose Or Reuse A Narrow Route 2 View

Goal: Make the selected consumer able to read Route 2 facts through a narrow,
fail-closed semantic surface.

Primary target:
- Existing Route 2 index/query helpers or a small validated adapter around
  them.

Actions:
- Prefer existing Route 2 query APIs and indexes before adding a new wrapper.
- If a wrapper is needed, key it by the selected consumer's existing semantic
  handle and return only select-root identity, root instruction index, scalar
  eligibility, and direct-global dependency presence.
- Preserve prepared helper visibility and fallback behavior.
- Add or update focused helper tests for direct-global present/absent,
  scalar-eligible/ineligible, nested select-chain, and fail-closed cases as
  needed for the selected surface.

Completion check:
- The selected consumer has a Route 2-readable semantic view, and helper-level
  tests prove agreement with prepared oracle behavior without weakening
  prepared tests.

### Step 3: Migrate The Selected Consumer

Goal: Switch exactly one selected AArch64 consumer to use Route 2 facts where
available.

Primary target:
- The selected consumer file from Step 1.

Actions:
- Replace the selected semantic prepared read with the Route 2 view where it
  answers the needed fact.
- Keep prepared fallback for absent Route 2 records or compatibility paths that
  still require it.
- Leave target-owned materialization, storage/home, memory operand, call, and
  final record decisions in the existing AArch64/prepared code.
- Do not touch unrelated consumer families.

Completion check:
- The selected consumer reads Route 2 semantic facts on the intended path,
  prepared fallback still works, and no unrelated prepared API contraction is
  claimed.

### Step 4: Prove Route/Prepared Equivalence

Goal: Show that the migrated consumer preserves behavior across the Route 2
cases required by the source idea.

Primary target:
- Existing prepared lookup helper tests and the narrow AArch64 backend test
  subset that exercises the selected consumer.

Actions:
- Add or adjust tests so the migrated consumer is covered for direct-global
  present and absent cases.
- Cover scalar-eligible and scalar-ineligible roots.
- Cover nested select-chain behavior.
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
  remaining Route 2 residual consumers discovered during execution.
- If the runbook is exhausted but the source idea still has residual scope,
  report that explicitly instead of closing the idea automatically.
- If implementation changed shared Route 2 or prepared helper APIs, recommend
  broader backend validation to the supervisor.

Completion check:
- `todo.md` states whether idea 185 is complete, blocked, or needs a follow-up
  route/lifecycle decision, with proof results attached.
