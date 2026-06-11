# PreparedFunctionLookups ownership/readiness audit

Status: Active
Source Idea: ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md

## Purpose

Build a field-by-field ownership and readiness map for
`PreparedFunctionLookups` after the Phase D selected-consumer route-view
migrations.

## Goal

Produce an audit artifact that classifies each lookup group by future
ownership, names residual readers, and identifies prerequisites for future
aggregate contraction or compatibility-adapter work.

## Core Rule

Do not rename, split, delete, or privatize `PreparedFunctionLookups` fields as
part of this audit. The job is to map actual ownership and residual coupling
before any Phase E retirement or aggregate contraction work proceeds.

## Read First

- `ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- The declaration and construction sites for `PreparedFunctionLookups`
- Production, printer/debug, target-wrapper, and oracle-test consumers that
  read `PreparedFunctionLookups` lookup groups

## Current Scope

- Inventory every `PreparedFunctionLookups` lookup group.
- Name current readers for each group, separated by production,
  printer/debug, target-wrapper, and oracle-test use.
- Classify each group as BIR annotation, transient pass context,
  target/prepared policy, compatibility adapter, retained oracle, or
  blocked/unknown.
- Identify lookup groups partially replaced by route views and the public
  fallback or oracle surfaces that still keep them coupled.
- Produce prerequisites for future aggregate contraction or adapter work.

## Non-Goals

- Do not rename `PreparedFunctionLookups`.
- Do not split the aggregate or move fields.
- Do not delete lookup groups or privatize accessors.
- Do not implement route migrations found by the audit.
- Do not fold `PreparedBirModule` construction or mutation retirement into
  this aggregate audit.
- Do not open draft 155.
- Do not weaken oracle coverage or test contracts to make a lookup group look
  ready.

## Working Model

- Semantic route facts can move toward route-native or BIR-owned ownership only
  when production and oracle readers agree on the source and fallback boundary.
- Target/prepared policy remains target/prepared-owned even when selected route
  consumers have migrated.
- Printer/debug and oracle readers may intentionally keep retained oracle
  surfaces public until replacement diagnostics or positive/negative coverage
  exists.
- Compatibility adapters are transitional surfaces, not proof that the
  underlying lookup group is retired.

## Execution Rules

- Work in audit-sized packets; do not combine inventory, classification, and
  durable documentation in one broad rewrite.
- Prefer `rg` and existing declaration sites to enumerate lookup groups before
  classifying ownership.
- Record unknowns explicitly instead of inferring ownership from a single
  selected consumer.
- If a lookup group needs implementation to retire, list it as a prerequisite
  or future idea instead of changing code under this plan.
- Treat classification-only edits as audit progress only; do not claim
  compiler/backend capability progress from them.

## Ordered Steps

### Step 1: Inventory lookup groups and readers

Goal: enumerate every `PreparedFunctionLookups` lookup group and the current
reader set for each group.

Primary targets:

- `PreparedFunctionLookups` declaration and construction sites
- Accessors, direct field readers, and helper wrappers that expose lookup
  groups
- Production, printer/debug, target-wrapper, and oracle-test consumers

Actions:

- Locate the aggregate declaration and all field names.
- Locate construction, mutation, and publication paths for each lookup group.
- Group readers by production, printer/debug, target-wrapper, oracle-test, and
  unknown use.
- Mark whether each reader consumes the lookup group directly, through a
  helper, or through a compatibility adapter.

Completion check:

- `todo.md` names every discovered lookup group and has at least one reader
  classification or an explicit justified `unknown` for each group.

### Step 2: Classify ownership and readiness

Goal: assign each lookup group a future ownership/readiness classification
without hiding residual coupling.

Primary targets:

- Lookup-group inventory from Step 1
- Existing route-view migration notes and Phase D readiness documents

Actions:

- Classify each group as BIR annotation, transient pass context,
  target/prepared policy, compatibility adapter, retained oracle, or
  blocked/unknown.
- Distinguish semantic route facts from target/prepared policy.
- Name partial route-view replacements and the fallback/oracle surfaces that
  keep each lookup group public.
- Record why any group is blocked or unknown.

Completion check:

- The active notes classify every lookup group and name the specific residual
  reader or missing evidence that prevents deletion or privatization.

### Step 3: Identify adapter and contraction prerequisites

Goal: make future aggregate contraction work actionable without changing the
aggregate in this plan.

Primary targets:

- Lookup groups classified as route-native candidates, compatibility adapters,
  retained oracles, or blocked/unknown

Actions:

- For route-native or adapter candidates, name the required adapter boundary
  and proof surface.
- For retained oracle groups, name the replacement diagnostics or coverage
  needed before contraction.
- For target/prepared policy groups, state why ownership should remain there.
- For blocked/unknown groups, name the missing evidence and the smallest
  follow-up that could resolve it.

Completion check:

- Each lookup group has either a contraction prerequisite, a retained-ownership
  reason, or a concrete blocked/unknown follow-up.

### Step 4: Produce the durable ownership/readiness map

Goal: leave a durable audit artifact that future draft 155 or
`PreparedBirModule` retirement analysis can cite.

Primary targets:

- A docs artifact under `docs/bir_prealloc_fusion/`, selected using nearby
  naming conventions
- `todo.md` for packet progress and docs-only proof notes

Actions:

- Write the map with columns for lookup group, current readers, ownership
  classification, route-view replacement status, fallback/oracle surfaces,
  adapter need, blockers, and future readiness.
- Include a short summary separating semantic route facts from target/prepared
  policy and retained oracle surfaces.
- Include prerequisites for future aggregate contraction or compatibility
  adapter work.
- Do not edit the source idea unless lifecycle policy requires a compact
  durable update.

Completion check:

- The artifact satisfies the source idea acceptance criteria and gives future
  retirement analysis enough evidence to choose detailed field-level work.

### Step 5: Validate lifecycle and audit boundaries

Goal: confirm the audit did not drift into implementation, broad
`PreparedBirModule` retirement, or testcase-overfit work.

Actions:

- Confirm no implementation files, tests, unsupported markers, or expectations
  changed unless the supervisor explicitly authorized a follow-up packet.
- Confirm every lookup group has a classification or justified unknown status.
- Confirm residual production, printer/debug, target-wrapper, and oracle
  readers are named where present.
- Record docs-only validation in `todo.md` if this plan stays audit-only.

Completion check:

- The supervisor can decide whether idea 196 is complete or whether it should
  spawn narrower implementation or evidence-gathering ideas before draft 155.
