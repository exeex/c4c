# Phase E Route 6 Call-Use Source View Consumer Migration

Status: Active
Source Idea: ideas/open/187_phase_e_route6_call_use_source_view_consumer_migration.md

## Purpose

Migrate one AArch64 scalar call argument or result source reader to the Route 6
call-use source view while keeping prepared call surfaces public as fallback
and oracle surfaces.

## Goal

Switch exactly one selected call argument/result source-producer or
publication-source materialization role class to Route 6 semantic source facts
where available.

## Core Rule

Route 6 may expose only semantic call-use source facts. Do not move ABI
register or stack placement, variadic FPR counts, clobber/preserve policy,
byval lane handling, outgoing stack sizing, helper resources, carrier
protocols, late-publication mechanics, call record spelling, or broad prepared
aggregate ownership into the Route 6 view.

## Read First

- `ideas/open/187_phase_e_route6_call_use_source_view_consumer_migration.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Prepared call-plan, argument/result plan, publication-routing,
  call-argument source-producer, value-home, move-bundle, and call-boundary
  effect helpers.
- Relevant AArch64 call lowering files discovered from the selected path, such
  as `src/backend/mir/aarch64/codegen/calls.cpp` and adjacent call-source,
  publication-source, value-home, or move-bundle consumers.

## Current Targets / Scope

- One AArch64 scalar call argument or result source-producer reader, or one
  publication-source materialization reader for a call argument/result role.
- A Route 6 view keyed by call instruction plus argument/result role and value
  role.
- Prepared call-plan, argument/result plan, publication-routing,
  call-argument source-producer, value-home, move-bundle, and call-boundary
  effect helpers retained as public fallbacks and equivalence oracles.

## Non-Goals

- Do not delete or hide prepared call surfaces.
- Do not migrate multiple argument/result role classes in this plan.
- Do not rewrite expectation contracts to claim progress.
- Do not add named-case shortcuts, broad BIR rescans, predecessor rescans, or
  name matching to reconstruct missing Route 6 facts.
- Do not move ABI placement, helper selection, carrier protocols,
  late-publication mechanics, outgoing stack sizing, or final call record
  spelling into BIR or the Route 6 view.

## Working Model

- Route 6 records and indexes already provide target-neutral semantic source
  relationships for call uses.
- Prepared call helpers remain necessary for residual consumers and as
  equivalence oracles during migration.
- The migrated consumer should read Route 6 first when a valid semantic record
  is available and preserve prepared fallback behavior when Route 6 is absent,
  invalid for the selected role, or still needs compatibility behavior.
- Target-owned call ABI and machine-record decisions stay in prepared/AArch64
  code.

## Execution Rules

- Keep each implementation packet bounded to one call argument/result role
  class or one test equivalence surface.
- Before editing, identify the prepared helper call being replaced and the
  corresponding Route 6 call-use source query.
- Preserve prepared oracle tests and add route/prepared agreement coverage
  instead of weakening expectations.
- Cover direct source, publication source, missing source, and recursive
  operand fallback behavior for the selected role class.
- Use build proof for every code-changing packet, then the
  supervisor-selected narrow backend test subset. Escalate to broader backend
  validation if public helper signatures, call traversal context wiring,
  publication-routing policy, or shared lookup APIs change.
- Treat testcase-shaped matching, unsupported downgrades, and expectation-only
  changes as route drift.

## Ordered Steps

### Step 1: Select One Route 6 Call-Use Consumer

Goal: Identify the narrowest AArch64 call argument/result role class that can
read Route 6 source facts without importing call ABI policy into BIR.

Primary target:
- One prepared scalar call argument source-producer, result source-producer, or
  publication-source materialization reader in AArch64 call lowering.

Actions:
- Inspect current prepared helper users for call argument/result source facts,
  publication-source materialization, value-home lookups, move bundles, and
  call-boundary effects.
- Choose one role class with nearby Route 6 oracle coverage and a clear
  prepared fallback story.
- Record the chosen prepared helper, Route 6 query surface, and proof subset in
  `todo.md` before implementation begins.
- Do not choose a path that requires moving ABI register/stack placement,
  helper resources, carrier protocols, byval lane handling, variadic FPR
  counts, outgoing stack sizing, clobber/preserve policy, late publication, or
  call record spelling into Route 6.

Completion check:
- `todo.md` names one selected call-use consumer path, its prepared
  fallback/oracle, and the narrow proof command the supervisor should delegate
  to an executor.

### Step 2: Expose Or Reuse A Narrow Route 6 View

Goal: Make the selected consumer able to read Route 6 call-use source facts
through a narrow, fail-closed semantic surface.

Primary target:
- Existing Route 6 index/query helpers or a small validated adapter around
  them.

Actions:
- Prefer existing Route 6 query APIs and indexes before adding a new wrapper.
- If a wrapper is needed, key it by call instruction plus argument/result role
  and value role, and return only source-producer or publication-source
  relationships needed by the selected consumer.
- Preserve prepared helper visibility and fallback behavior.
- Add or update focused helper tests for present route data, absent route data,
  invalid or mismatched references, recursive operand fallback, and prepared
  agreement as needed for the selected surface.

Completion check:
- The selected consumer has a Route 6-readable semantic view, and helper-level
  tests prove agreement with prepared oracle behavior without weakening
  prepared tests.

### Step 3: Migrate The Selected Consumer

Goal: Switch exactly one selected AArch64 call-use consumer to use Route 6
facts where available.

Primary target:
- The selected AArch64 call lowering consumer file from Step 1.

Actions:
- Replace the selected semantic prepared read with the Route 6 view where it
  answers the needed fact.
- Keep prepared fallback for absent Route 6 records, invalid role references,
  recursive operand recovery, or compatibility paths that still require it.
- Leave ABI placement, helper selection, carrier handling, call-boundary
  effects, late publication, move emission, and final call record spelling in
  existing AArch64/prepared code.
- Do not touch unrelated call argument/result role classes.

Completion check:
- The selected consumer reads Route 6 semantic source facts on the intended
  path, prepared fallback still works, and no unrelated prepared API
  contraction is claimed.

### Step 4: Prove Route/Prepared Equivalence

Goal: Show that the migrated consumer preserves behavior across the Route 6
cases required by the source idea.

Primary target:
- Existing prepared call helper tests, Route 6 oracle tests, and the narrow
  AArch64 backend test subset that exercises the selected consumer.

Actions:
- Add or adjust tests for direct source, publication source, missing source,
  invalid-reference, and recursive operand fallback cases where relevant to the
  selected role class.
- Keep call-plan and ABI policy tests active to prove call ABI ownership did
  not move into BIR.
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
  residual Route 6 call-use consumers discovered during execution.
- If the runbook is exhausted but the source idea still has residual scope,
  report that explicitly instead of closing the idea automatically.
- If implementation changed shared Route 6 or prepared call helper APIs,
  recommend broader backend validation to the supervisor.

Completion check:
- `todo.md` states whether idea 187 is complete, blocked, or needs a follow-up
  route/lifecycle decision, with proof results attached.
