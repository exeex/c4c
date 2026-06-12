# Phase D2 Retained-Surface Consumer Switch Analysis Runbook

Status: Active
Source Idea: ideas/open/203_phase_d2_retained_surface_consumer_switch_analysis.md

## Purpose

Transcribe the Phase D2 idea into an execution runbook for an analysis-only pass over C2-retained BIR/prealloc surfaces.

## Goal

Produce `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md` and any justified one-surface follow-up idea files without migrating consumers, deleting prepared APIs, or claiming cache/API contraction readiness.

## Core Rule

Preserve C2 retained-surface decisions unless this analysis records new surface-specific evidence from code, required documents, or closure notes.

## Read First

- `ideas/open/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`
- `ideas/closed/201_phase_b2_selected_route_extension_schema_readiness_audit.md`
- `ideas/closed/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md`
- `ideas/closed/202_route3_memory_source_identity_adapter.md`
- `ideas/closed/203_route4_publication_identity_adapter.md`
- `ideas/closed/204_route5_edge_join_source_adapter.md`
- `ideas/closed/205_route6_call_use_source_adapter.md`
- `ideas/closed/206_route7_comparison_provenance_diagnostic_oracle.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- accepted baseline state and recent baseline-recovery closure notes, if present.

## Current Targets

- Durable analysis output: `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- Source lifecycle idea: `ideas/open/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- Optional accepted follow-up ideas under `ideas/open/`, each scoped to exactly one prepared surface or diagnostic/oracle row.

## Non-Goals

- Do not implement consumer migrations.
- Do not delete prepared APIs, privatize caches, or contract aggregate APIs.
- Do not open or execute draft 155.
- Do not start broad `PreparedFunctionLookups` or `PreparedBirModule` retirement.
- Do not weaken tests, diagnostics, baselines, helper-oracle names, supported-path status, or expected strings.
- Do not move target/prepared policy into route ownership without a named replacement owner and proof plan.

## Working Model

- C2 retained surfaces are starting facts, not failures to bypass.
- Each plausible follow-up must be classified as one of:
  - semantic reader migration;
  - diagnostic/oracle replacement;
  - public fallback removal after replacement evidence;
  - retained-owner adapter clarification;
  - target/prepared policy ownership migration;
  - no-action because the surface is aggregate, policy-owned, or still blocked.
- Each accepted implementation follow-up must name exactly one prepared surface or diagnostic/oracle row, its retained fallback or policy boundary, the evidence route fact, and the proof shape required before any later contraction claim.

## Execution Rules

- Keep this phase analysis-only.
- Prefer exact code/document references over broad summaries.
- Keep Route 3 through Route 7 separate when recording evidence.
- Treat printer/debug output, wrapper compatibility, diagnostics, baselines, and expected strings as authority surfaces, not optional proof.
- Record no-action decisions explicitly for fallback/oracle, target/prepared policy, pass-context, and blocked aggregate surfaces.
- When creating follow-up ideas, include concrete reviewer reject signals against testcase-shaped shortcuts, broad rewrites, expectation weakening, and old failure modes hidden behind new names.

## Ordered Steps

### Step 1: Gather Retained-Surface Evidence

Goal: Build the raw evidence set from required inputs before inspecting implementation consumers.

Primary targets:
- Required documents and closure notes listed in `Read First`
- Baseline-recovery closure notes, if discoverable

Actions:
- Read the required inputs and extract C2 classifications for Route 3 through Route 7 retained surfaces.
- Record exact retained prepared surfaces, diagnostic/oracle rows, target/prepared policy boundaries, pass-context areas, and blocked aggregate surfaces.
- Note any closure evidence that constrains later follow-up scope.

Completion check:
- A working evidence list exists in the D2 analysis draft with route, surface, C2 classification, retained boundary, and source reference for each candidate.

### Step 2: Map Current Consumers

Goal: Build the retained-surface consumer dependency map required by the source idea.

Primary targets:
- Production consumers
- Printer/debug consumers
- Target-wrapper consumers
- Oracle and expected-string consumers
- Pass-context and aggregate coupling points

Actions:
- Inspect code and tests only to identify current consumers and authorities.
- For each candidate surface, record whether selected Route 3 through Route 7 adapter work removed any public consumer boundary or only added route/prepared agreement.
- Separate semantic identity surfaces from publication mechanics, move/edge publication policy, ABI/call-plan policy, branch policy, printer/debug, and string authority surfaces.

Completion check:
- The D2 analysis draft contains a dependency-map row for each plausible candidate with current consumers, retained fallback/policy/oracle authority, and adapter effect.

### Step 3: Classify Follow-Up Viability

Goal: Decide which surfaces deserve one-surface follow-up ideas and which remain no-action.

Primary target:
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`

Actions:
- For each candidate, classify next work using the allowed categories from the source idea.
- Accept follow-up work only when it can remain one surface or one diagnostic/oracle row.
- Reject broad migration ladders, aggregate retirement, draft 155 readiness, and prepared-module retirement claims unless a separate field-by-field prerequisite is documented as future analysis.
- Write explicit no-action decisions for aggregate, policy-owned, pass-context, and blocked surfaces.

Completion check:
- The D2 analysis document has clear accepted/no-action decisions and explains remaining blockers to `PreparedFunctionLookups` and `PreparedBirModule` retirement.

### Step 4: Define Proof Shapes

Goal: Attach proof expectations to every accepted follow-up before creating implementation ideas.

Primary target:
- D2 analysis document and any accepted follow-up idea drafts

Actions:
- For each accepted follow-up, specify positive, absent, invalid, duplicate/conflict, mismatch, fallback, printer/debug, wrapper, and expected-string cases as applicable.
- Name the retained fallback or policy boundary and the route fact or retained-owner evidence to use.
- State that no later contraction claim is valid without the named proof shape.

Completion check:
- Every accepted follow-up has a one-surface scope, retained authority boundary, evidence source, and proof plan.

### Step 5: Write Durable Outputs

Goal: Finish the analysis artifact and lifecycle-ready follow-up ideas.

Primary targets:
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- Optional `ideas/open/*.md` follow-up ideas
- `todo.md`

Actions:
- Write the final D2 analysis document with a linkable closure-ready structure.
- Create only justified follow-up idea files, each with goal, scope, acceptance criteria, and reviewer reject signals.
- Update `todo.md` with the completed packet, proof performed, and the next lifecycle recommendation.

Completion check:
- The analysis document exists, any follow-up ideas are one-surface scoped, and `todo.md` records proof and completion state for supervisor review.

## Validation

- This is docs and lifecycle analysis work; no code build is required unless implementation files are modified by mistake.
- Required proof is review evidence in the D2 document: cited required inputs, consumer references, accepted/no-action classifications, proof shapes, and explicit non-readiness statements for draft 155, broad `PreparedFunctionLookups`, and broad `PreparedBirModule` retirement.
- If any implementation file changes, stop and return to the supervisor because that violates this runbook's analysis-only scope.
