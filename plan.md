# Phase E0 Prepared/BIR Merge Readiness Runbook

Status: Active
Source Idea: ideas/open/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md

## Purpose

Prepare the ownership boundary map for a realistic Prepared/BIR merge strategy.
This is Phase E0 of the BIR/prealloc thinning program and is analysis-only.

## Goal

Produce `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
with a field-by-field and group-by-group readiness map that separates BIR-owned
semantics, retained prepared/target policy, public fallback/oracle surfaces,
and duplicate semantic helper deletion candidates.

## Core Rule

Do not implement, delete, privatize, rename, or weaken tests as part of this
plan. The accepted output is durable analysis that enables later E1-E5 work
without treating facade moves or container reshuffles as code reduction.

## Read First

- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`
- `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`
- `ideas/closed/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md`
- `ideas/closed/192_residual_route_view_consumer_migration_map.md`
- `ideas/closed/203_phase_d2_retained_surface_consumer_switch_analysis.md`
- `ideas/closed/207_route3_memory_source_semantic_reader.md`
- `ideas/closed/208_route3_memory_source_oracle_printer_row.md`
- `ideas/closed/209_route4_publication_source_semantic_reader.md`
- `ideas/closed/210_route4_block_entry_publication_printer_debug_row.md`
- `ideas/closed/211_route5_current_block_join_source_semantic_reader.md`
- `ideas/closed/212_route5_edge_join_oracle_printer_row.md`
- `ideas/closed/213_route6_call_source_consumer.md`
- `ideas/closed/214_route6_x86_scalar_source_route_debug_row.md`
- `ideas/closed/215_route7_comparison_provenance_consumer.md`
- `ideas/closed/216_route7_comparison_oracle_row.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`

## Current Targets

- `PreparedBirModule` field ownership.
- `PreparedFunctionLookups` lookup-group ownership.
- Selected Route 1-8 semantic facts versus prepared policy/fallback.
- Public prepared API surfaces that may later contract into private pass
  context.
- Duplicate prepared semantic helpers that overlap existing BIR route facts,
  indexes, identities, or annotations.
- Diagnostic, printer/debug, route-debug, helper-oracle, and expected-string
  authority.
- AArch64, x86, and riscv consumer boundaries as readiness evidence.
- Follow-up recommendations for E1 through E5.

## Non-Goals

- Do not edit implementation files.
- Do not open or rewrite draft 155.
- Do not directly delete or privatize prepared APIs.
- Do not move target ABI, layout, register, stack, emission, or wrapper policy
  into target-neutral BIR.
- Do not claim code-size progress from facade renames, wrapper reshuffles, or
  aggregate container moves.
- Do not weaken tests, diagnostics, baselines, helper-oracle names,
  supported-path status, or expected strings.

## Working Model

Use these classifications for every relevant field, lookup group, helper, or
API surface:

- BIR-owned semantic fact/index/identity candidate.
- BIR annotation candidate.
- Duplicate semantic helper deletion/contraction candidate.
- Private/transient pass context.
- Retained target/prepared policy.
- Retained public fallback/oracle.
- Diagnostic/string authority.
- Cross-target compatibility boundary.
- Blocked/unknown pending more evidence.

For code-size-positive candidates, record the exact helper or API family that
could eventually be deleted, the BIR fact that would replace its semantic
authority, and the prepared fallback/policy/oracle surface that must remain
until later proof exists.

## Execution Rules

- Treat Phase D, D2, and Route 1-8 closure notes as evidence, not as broad
  prepared-retirement permission.
- Prefer source citations and concrete field/helper names over general claims.
- Preserve target-policy ownership unless a source explicitly names a
  target-neutral replacement.
- Keep diagnostics, printer/debug rows, route-debug rows, helper oracles, and
  expected-string authority visible as retained blockers or prerequisites.
- Reject testcase-shaped shortcuts, expectation downgrades, and any analysis
  that treats selected reader migration as proof of broad API retirement.
- Write routine progress and proof notes in `todo.md`; do not edit the source
  idea unless durable source intent changes.

## Ordered Steps

### Step 1: Build the Evidence Inventory

Goal: establish the input set and extract the concrete ownership surfaces that
need classification.

Primary targets:

- Required idea and docs inputs listed above.
- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`

Actions:

- Read every required input.
- Inventory all `PreparedBirModule` fields and all `PreparedFunctionLookups`
  lookup groups.
- Capture the closure-note constraints from Phase D, Phase D2, and Route 1-8.
- Identify consumer-boundary evidence for AArch64, x86, and riscv.

Completion check:

- `todo.md` names the complete inventory sources, the discovered field/group
  lists, and any missing or ambiguous evidence before classification begins.

### Step 2: Classify `PreparedBirModule` Ownership

Goal: produce a field-by-field ownership map for `PreparedBirModule`.

Primary target:

- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`

Actions:

- Classify every `PreparedBirModule` field using the working-model categories.
- Distinguish BIR semantic facts, BIR annotation candidates, target/prepared
  policy, public fallback/oracle surfaces, diagnostics, and unknowns.
- For each code-size-positive candidate, name the duplicate helper/API family,
  replacement BIR fact, retained fallback/policy/oracle, and proof required
  before deletion.

Completion check:

- The durable doc contains a complete `PreparedBirModule` field-by-field map
  with no target-policy data reclassified as BIR-owned only to make deletion
  appear easier.

### Step 3: Classify `PreparedFunctionLookups` Ownership

Goal: produce a lookup-group ownership map for `PreparedFunctionLookups`.

Primary target:

- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`

Actions:

- Classify every lookup group using the working-model categories.
- Identify groups that duplicate selected Route 1-8 semantic facts, indexes,
  identities, or annotations.
- Mark retained public fallback/oracle, diagnostic/string authority, and
  cross-target compatibility boundaries explicitly.
- Identify public prepared APIs that are realistic E2 private/pass-context
  contraction candidates.

Completion check:

- The durable doc contains a complete `PreparedFunctionLookups` group-by-group
  map and separates deletion/contraction candidates from retained policy and
  fallback surfaces.

### Step 4: Synthesize E1-E5 Readiness

Goal: convert the maps into a later-phase boundary and recommendation table.

Primary target:

- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`

Actions:

- Build a target-end-state table organized by:
  BIR owns semantics, prepared owns policy, prepared APIs become private pass
  context, and duplicate semantic helpers are deletion candidates.
- List semantic duplicate surfaces that are candidates for E1.
- List public prepared APIs that are candidates for E2.
- List helper/API families where code-size reduction is realistic and the
  proof required before deletion.
- List surfaces where code-size reduction is not realistic because the surface
  is target policy, diagnostics/oracle, cross-target compatibility, or retained
  fallback.
- Name diagnostic/oracle replacement prerequisites for E3.
- Name cross-target wrapper blockers for E4.
- Define exact criteria for when draft 155 / E5 can be rewritten or opened.

Completion check:

- The durable doc states a concrete E1-E5 readiness path and rejects broad
  prepared retirement until field ownership, fallback/oracle, diagnostic,
  string, and public-consumer blockers are mapped.

### Step 5: Validate Analysis Completeness

Goal: make the analysis acceptance-ready without implementation changes.

Primary target:

- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`

Actions:

- Check the durable doc against every expected-output bullet in the source
  idea.
- Check reviewer reject signals explicitly:
  selected reader proof is not treated as broad retirement proof, target policy
  is not moved into BIR, facade/container moves are not claimed as code
  reduction, printer/debug/oracle surfaces remain visible, draft 155 is not
  recommended prematurely, duplicate semantic helper families are the primary
  deletion target, and no implementation is mixed into the analysis.
- Run a docs-only proof appropriate for the final changed files, or state why
  no executable proof applies.

Completion check:

- `todo.md` records the final proof and the durable doc links back to the
  active source idea requirements.
