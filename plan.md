# Phase E1 Semantic Duplicate Candidate Triage Runbook

Status: Active
Source Idea: ideas/open/218_phase_e1_semantic_duplicate_candidate_triage.md

## Purpose

Turn the Phase E0 semantic-duplicate candidate set into narrowly scoped
follow-up ideas, one duplicate semantic helper family or one row-scoped
semantic/oracle surface at a time.

## Goal

Produce `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
with candidate-by-candidate readiness buckets, deferrals, no-action decisions,
and proof requirements for safe later E1 follow-up work.

## Core Rule

This activation is analysis-only. Do not implement helper migrations, delete
prepared APIs, privatize public prepared surfaces, move target policy into BIR,
open draft 155 / E5, or treat E0 candidate categories as implementation
permission.

## Read First

- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`
- `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- Route 3 through Route 7 closure notes `207` through `216`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- current accepted baseline state

## Current Targets

- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- aggregate BIR semantic forwarding
- control-flow identity helpers
- route source/publication/join/call/comparison identity helpers
- liveness identity helpers
- intrinsic metadata adapters
- row-scoped diagnostic/oracle helpers

## Non-Goals

- Do not edit implementation files.
- Do not directly delete helpers or prepared APIs.
- Do not privatize public prepared APIs; defer that to E2 after selected E1
  semantic replacements are proven.
- Do not replace broad prepared printer/debug/oracle surfaces; defer that to E3.
- Do not migrate cross-target wrappers; defer that to E4.
- Do not design Route 8 return-chain owner/schema beyond explicit deferral.
- Do not move ABI, layout, registers, stack, address materialization, move
  scheduling, branch spelling, helper protocols, final records,
  target-wrapper behavior, or emitted output into target-neutral BIR.
- Do not weaken tests, diagnostics, baselines, helper-oracle names,
  supported-path status, or expected strings.

## Working Model

Classify each E0 candidate family into exactly one readiness owner:

- ready to draft one implementation idea;
- needs more analysis before implementation;
- defer to E2 for public API/private pass-context contraction;
- defer to E3 for diagnostic/oracle replacement;
- defer to E4 for cross-target wrapper convergence;
- blocked because the surface is target/prepared policy, fallback/oracle, or
  aggregate compatibility.

For each family, identify the exact helper/API family, the BIR fact or
annotation that would own the semantics, the prepared fallback or policy that
must remain, public consumers still present, and the proof shape required
before code deletion or helper contraction is allowed.

## Execution Rules

- Treat E0 as evidence, not implementation permission.
- Keep follow-up ideas scoped to one semantic duplicate helper family or one
  row-scoped semantic/oracle surface.
- Count only code-size-positive deletion or contraction that removes duplicate
  semantic authority, not facade renames, wrapper moves, or container
  reshuffles.
- Preserve prepared fallback, target/prepared policy, diagnostics, helper
  oracles, expected strings, and cross-target compatibility as retained
  blockers unless equivalent proof exists.
- Record routine progress and proof in `todo.md`; do not edit the source idea
  unless durable source intent changes.

## Ordered Steps

### Step 1: Build the E1 Evidence Inventory

Goal: establish the complete input set and extract the E0 candidate evidence
before classification begins.

Primary targets:

- Required inputs listed in `Read First`.
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`

Actions:

- Read every required input, including Route 3 through Route 7 closure notes
  `207` through `216`.
- Capture the E0 target end state and candidate families.
- Extract helper/API names, BIR replacement facts or annotations, retained
  fallback/policy/oracle surfaces, public consumers, and baseline constraints.
- Create the durable E1 document skeleton with source idea linkage.

Completion check:

- `todo.md` records the complete input inventory and any missing or ambiguous
  evidence before readiness buckets are assigned.
- The durable E1 document skeleton exists and names the six candidate families.

### Step 2: Classify the Six Candidate Families

Goal: assign each E0 E1 family to the correct readiness bucket with explicit
evidence.

Primary target:

- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`

Actions:

- Classify aggregate BIR semantic forwarding.
- Classify control-flow identity helpers.
- Classify route source/publication/join/call/comparison identity helpers.
- Classify liveness identity helpers.
- Classify intrinsic metadata adapters.
- Classify row-scoped diagnostic/oracle helpers.
- For each family, name the exact helper/API surface, semantic owner,
  retained prepared surface, public consumers, and proof shape.

Completion check:

- The durable E1 document contains a candidate-by-candidate triage table for
  all six families.
- No family is treated as ready for implementation merely because E0 named it
  as a candidate category.

### Step 3: Draft Accepted Follow-Up Ideas

Goal: turn only ready E1 surfaces into narrowly scoped follow-up ideas.

Primary targets:

- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- new `ideas/open/*.md` files only for accepted follow-ups

Actions:

- Draft one follow-up idea per accepted semantic duplicate helper family or
  row-scoped semantic/oracle surface.
- Include concrete in-scope work, out-of-scope work, proof requirements, and
  reviewer reject signals for each follow-up.
- Do not open broad prepared retirement, aggregate `PreparedBirModule` /
  `PreparedFunctionLookups` retirement, or draft 155 / E5 work.

Completion check:

- Every accepted follow-up idea is scoped to exactly one helper family or one
  row-scoped semantic/oracle surface.
- The durable E1 document links the accepted follow-up idea files and explains
  why each is ready.

### Step 4: Record Deferrals and No-Action Decisions

Goal: make non-E1 ownership explicit so later work does not drift into broad
prepared retirement.

Primary target:

- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`

Actions:

- Record explicit deferrals to E2, E3, E4, or Route 8 analysis where E1 is not
  the right owner.
- Record no-action decisions for aggregate, target-policy, fallback/oracle,
  diagnostic/string, and cross-target compatibility surfaces.
- State that draft 155 / E5 remains unopened.

Completion check:

- The durable E1 document separates ready follow-ups from E2/E3/E4/Route 8
  deferrals, blockers, and no-action decisions.
- No target/prepared policy, fallback/oracle, diagnostic/string, or
  cross-target compatibility surface is reclassified as a semantic deletion
  candidate to force progress.

### Step 5: Validate E1 Analysis Completeness

Goal: make the analysis acceptance-ready without implementation changes.

Primary target:

- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`

Actions:

- Check the durable document against every expected-output bullet in the source
  idea.
- Check reviewer reject signals explicitly: E0 categories are not treated as
  implementation permission, broad migration plans are not opened, facade or
  wrapper reshuffles are not counted as code reduction, target/prepared policy
  stays outside BIR, fallback/oracle surfaces are preserved until proof exists,
  diagnostics and expected strings are not weakened, and draft 155 / E5 remains
  unopened.
- Run a docs/lifecycle-appropriate proof, or state why no executable proof
  applies.

Completion check:

- `todo.md` records final proof and any residual follow-up notes.
- The durable E1 document links back to the active source idea requirements
  and satisfies the expected-output checklist.
