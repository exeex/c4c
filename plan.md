# Phase B2 Selected Route Extension Schema Readiness Audit Runbook

Status: Active
Source Idea: ideas/open/201_phase_b2_selected_route_extension_schema_readiness_audit.md

## Purpose

Audit the Phase A2-accepted Route 1-8 extension/import candidates and decide
whether each one needs a small BIR schema/index extension, a compatibility or
diagnostic/oracle follow-up, retained-policy cleanup, or no schema action.

## Goal

Produce `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
as an analysis-only readiness artifact with bounded follow-up ideas only where
the evidence supports them.

## Core Rule

Do not implement schema records, indexes, adapters, diagnostics, MIR/codegen
migrations, prepared API contraction, or `PreparedBirModule` retirement. Keep
Phase B2 as an audit of selected existing Route 1-8 extension/import candidates,
not a new residual-route schema program.

## Read First

- `ideas/open/201_phase_b2_selected_route_extension_schema_readiness_audit.md`
- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`
- `ideas/closed/199_full_suite_baseline_string_authority_timeout_attribution.md`

## Current Targets

- Durable output:
  `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- Optional bounded follow-up ideas under `ideas/open/` only for candidates with
  enough evidence and a clear proof route.
- Closure note content when the source idea is ready to close.

## Non-Goals

- Do not open or execute draft 155.
- Do not create broad new BIR routes without evidence that contradicts Phase A2.
- Do not copy prepared wrapper structs or aggregate fields into BIR records.
- Do not encode target policy, ABI/layout, printer formatting, pass context,
  storage, move scheduling, branch policy, helper protocols, or final emission
  order as BIR schema.
- Do not delete, privatize, rename, or contract prepared helpers.
- Do not create a BIR-owned clone of `PreparedFunctionLookups` or
  `PreparedBirModule`.

## Working Model

- Phase A2 accepted only narrow existing Route 1-8 extension/import candidates.
- A candidate may need no schema work if existing route records/indexes already
  carry the target-neutral semantic identity.
- A candidate may need a small schema/index extension only when the missing fact
  is target-neutral semantic identity inside an existing Route 1-8 boundary.
- A candidate should become adapter, diagnostic/oracle, retained-policy cleanup,
  or no-action work when its missing fact belongs outside BIR schema.

## Execution Rules

- Preserve the Phase A2 excluded target/prepared policy for every candidate.
- Record negative, ambiguous, mismatch, invalid-reference, duplicate/conflict,
  policy-sensitive, and string-authority cases before recommending
  implementation.
- Treat prepared fallback/oracle coverage and string-authority discipline as
  first-class readiness requirements.
- Prefer one analysis artifact update plus focused follow-up idea files over
  broad plan expansion.
- For docs-only changes, proof can be inspection plus a targeted search for
  stale links or contradictory route classifications.

## Ordered Steps

### Step 1: Establish Evidence Matrix

Goal: Build the source evidence set for the audit without changing code.

Primary Target:
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`

Actions:
- Read every required artifact listed in `Read First`.
- Extract the Phase A2 accepted candidate rows for Routes 1-8.
- For each candidate, capture the semantic fact, accepted boundary, excluded
  target/prepared policy, and retained fallback/oracle surface.
- Create the initial analysis artifact with a per-route matrix skeleton if it
  does not already exist.

Completion Check:
- The artifact names every Route 1-8 accepted candidate from the source idea and
  has no broad residual-route or implementation language.

### Step 2: Classify Existing Schema Sufficiency

Goal: Decide whether existing route records/indexes are sufficient for each
candidate.

Primary Target:
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`

Actions:
- For each route candidate, inspect existing Phase A and Phase B route records,
  indexes, owner scopes, validation states, and fail-closed behavior.
- Mark each candidate as one of: existing schema sufficient, small
  schema/index extension, compatibility adapter, diagnostic/oracle replacement,
  retained-policy cleanup, or no action.
- Record missing field/id/index/validation/fail-closed details only where they
  are schema-relevant and target-neutral.

Completion Check:
- Every route has a schema-readiness classification with an explicit reason and
  a retained prepared fallback/oracle note.

### Step 3: Define Proof And Reject Coverage

Goal: Convert each classification into bounded proof expectations and reject
signals.

Primary Target:
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`

Actions:
- Add negative, ambiguous, mismatch, invalid-reference, duplicate/conflict,
  policy-sensitive, and string-authority cases where relevant.
- Identify the minimum proof route for each accepted schema/index extension or
  adapter recommendation.
- Keep rejected target/prepared policy out of BIR and explain why it remains
  outside schema scope.

Completion Check:
- Each route row includes proof recommendations and concrete reviewer reject
  signals that would block a future implementation slice.

### Step 4: Open Bounded Follow-Up Ideas

Goal: Create only the follow-up ideas that are justified by the completed audit.

Primary Targets:
- `ideas/open/*.md`
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`

Actions:
- Create follow-up ideas only for candidates with enough evidence and bounded
  implementation or adapter proof routes.
- Include concrete reviewer reject signals in every new idea.
- In the audit artifact, list deferred non-schema candidates and where they
  should go instead.
- Do not create ideas for retained-policy cleanup unless the audit shows a
  separate bounded initiative is required.

Completion Check:
- Every accepted follow-up idea maps to a specific route candidate and every
  deferred/rejected candidate has a documented destination or no-action reason.

### Step 5: Prepare Lifecycle Closure Evidence

Goal: Make the source idea closable without treating runbook exhaustion as
automatic completion.

Primary Targets:
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `todo.md`
- `ideas/open/201_phase_b2_selected_route_extension_schema_readiness_audit.md`

Actions:
- Verify the audit artifact contains the required linkable payload: per-route
  table, accepted schema/index follow-ups, deferred non-schema candidates,
  rejected surfaces, bridge/oracle/fallback notes, and proof-route
  recommendations.
- Record inspection proof and any targeted stale-link or classification checks
  in `todo.md`.
- Ask the plan owner to decide closure only after the source idea's expected
  output is satisfied.

Completion Check:
- The source idea can be closed with a concise note linking the audit artifact,
  or the remaining source-level gaps are explicit in `todo.md`.
