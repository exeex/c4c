# Phase C Prealloc Private Cache Contraction Audit Runbook

Status: Active
Source Idea: ideas/open/153_phase_c_prealloc_private_cache_contraction_audit.md

## Purpose

Audit which remaining prealloc structures should become private caches or thin
pass internals now that BIR owns the relevant semantic relationships and
annotation schema.

## Goal

Produce a durable Phase C analysis artifact and follow-up idea list for bounded
cache privatization and public API contraction work.

## Core Rule

This plan is analysis-only. Do not delete caches, weaken prepared facts, move
target-local MIR lowering details into prealloc, or make consumers rebuild
expensive indexes manually.

## Read First

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `ideas/closed/151_phase_a_bir_normalization_candidate_audit.md`
- `ideas/closed/152_phase_b_bir_annotation_schema_candidate_audit.md`
- `ideas/closed/159_bir_producer_identity_annotation_schema.md`
- `ideas/closed/160_bir_select_chain_global_dependency_annotation_schema.md`
- `ideas/closed/161_bir_memory_access_identity_annotation_schema.md`
- `ideas/closed/162_bir_publication_availability_annotation_schema.md`
- `ideas/closed/163_bir_edge_join_source_annotation_schema.md`
- `ideas/closed/164_bir_call_use_source_annotation_schema.md`
- `ideas/closed/165_bir_comparison_condition_annotation_schema.md`
- `ideas/closed/166_bir_annotation_lookup_index_schema.md`

## Current Targets

- Durable artifact:
  `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Prealloc lookup surfaces:
  `src/backend/prealloc/prepared_lookups.*`
- Domain lookup headers and implementations introduced or cleaned by ideas
  137-150.
- Prealloc module and plan surfaces:
  `src/backend/prealloc/module.hpp`
  `src/backend/prealloc/*plans*`
- Stack layout prealloc internals:
  `src/backend/prealloc/stack_layout/`
- AArch64 consumers that still depend on prealloc query types directly.

## Non-Goals

- Do not perform implementation changes during this audit.
- Do not privatize or delete APIs as part of Phase C.
- Do not edit Phase A or Phase B closed ideas.
- Do not rewrite BIR annotation schema decisions from Phase B.
- Do not convert target-specific MIR/codegen obligations into prealloc-owned
  semantic facts.

## Working Model

- Treat Phase B closed routes as the source of truth for contraction readiness.
- For each Phase B route, decide whether prepared lookup, index, or facade
  surfaces are redundant, public migration oracles, private construction
  caches, blocked by residual consumers, or out of scope for contraction.
- Preserve public surfaces temporarily when they are still needed as migration
  oracles or when residual consumers have not moved to BIR annotations/indexes.
- Record each proposed cache contraction route with its Phase A/B prerequisite
  section and proof-route recommendation.

## Execution Rules

- Keep routine execution notes in `todo.md`.
- Write durable audit findings only to
  `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`.
- If the audit discovers a separate implementation initiative, record it as a
  follow-up idea under `ideas/open/` instead of expanding this plan into
  implementation work.
- For any later code-changing follow-up idea, require build proof plus a narrow
  consumer-focused test route, and escalate to broader validation when public
  headers or shared lookup contracts change.

## Ordered Steps

### Step 1: Read Prerequisite Phase Artifacts

Goal: build the Phase C source-of-truth map from Phase A/B artifacts and Phase B
implementation closure notes.

Primary target: prerequisite documents and closed ideas listed in `Read First`.

Concrete actions:

- Read the Phase A normalization artifact and closure note.
- Read the Phase B annotation schema artifact and closure note.
- Read each Phase B implementation follow-up closure note from ideas 159-166.
- Extract route names, implemented BIR annotation/index surfaces, residual
  consumers, oracle coverage, and explicit blockers.
- Record temporary notes in `todo.md`; do not edit the source idea.

Completion check:

- A route-by-route prerequisite map exists in `todo.md` or the draft artifact
  for Routes 1-7 plus the route index facade.
- Each route lists its Phase A/B prerequisite source and any Phase B residual
  called out by the closure notes.

### Step 2: Inventory Remaining Prealloc Surfaces

Goal: identify the prepared lookup, plan, stack layout, and public query
surfaces that may be contraction candidates.

Primary target: `src/backend/prealloc/` and direct AArch64 consumers.

Concrete actions:

- Inspect `src/backend/prealloc/prepared_lookups.*`.
- Inspect domain lookup headers and implementations introduced or cleaned by
  ideas 137-150.
- Inspect `src/backend/prealloc/module.hpp`, prealloc plan files, and
  `src/backend/prealloc/stack_layout/`.
- Inspect AArch64 consumers that still include or call prealloc query types.
- Group surfaces by semantic owner, construction-only cache role, consumer
  dependency, and public header exposure.

Completion check:

- The audit draft contains a grouped inventory of cache/index/public API
  surfaces and names the consumers that keep each public surface alive.

### Step 3: Classify Phase B Route Contraction Status

Goal: decide the contraction state for each Phase B schema route.

Primary target: Phase B routes and their corresponding prepared lookup or
facade surfaces.

Concrete actions:

- For each Phase B route, decide whether prepared remains public, should become
  private, is oracle-only, or is blocked.
- Classify known residuals:
  - Route 4 block-entry MIR consumer on older semantic PHI scan.
  - Route 5 current-block join-source public helper on prior semantic
    implementation.
  - Route 6 call-use source records/indexes without broad MIR/codegen consumer
    switch.
  - Route 7 target branch spelling, fused-compare legality, condition-code
    selection, hazards, emitted-register state, and final record order.
  - Route index reference facade coverage for selected Route 4 and Route 7
    migrations while Routes 1, 2, 3, 5, and 6 retain typed record/index shapes.
- Identify required BIR annotation prerequisites before each privatization.

Completion check:

- The Phase C artifact has a Phase B route coverage table with cache
  contraction status, blockers, and public/private/oracle-only disposition.

### Step 4: Draft Follow-Up Ideas And Proof Routes

Goal: convert audit classifications into bounded implementation initiatives
without performing them.

Primary target: follow-up idea content in the Phase C artifact.

Concrete actions:

- Name each concrete cache/index group that should become private.
- Name each cache/index group that must remain public temporarily and its
  blocker.
- Draft follow-up idea summaries for each bounded privatization or API
  contraction route.
- For each follow-up, specify prerequisite BIR annotation/index surfaces,
  consumer migration requirements, and proof-route recommendations.
- Include reject signals for overfit contractions, weakened prepared facts, and
  target-local detail leakage.

Completion check:

- The Phase C artifact contains concrete follow-up ideas with prerequisites,
  proof routes, and rejection criteria sufficient for later activation.

### Step 5: Finalize Phase C Artifact And Closure Summary

Goal: make Phase C directly consumable by Phase D and Phase E.

Primary target:
`docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`.

Concrete actions:

- Ensure the artifact links every proposed contraction route back to its Phase
  A/B prerequisite section.
- Ensure the artifact contains:
  - Phase B route coverage table.
  - private cache/index groups.
  - temporarily public cache/index groups and blockers.
  - follow-up ideas for each concrete cache surface.
  - BIR annotation prerequisites.
  - proof-route recommendations.
- Prepare a closure note summary that links the artifact and summarizes its
  table, private groups, public blockers, follow-ups, prerequisites, and proof
  routes.

Completion check:

- The artifact is complete enough for Phase D and Phase E to consume directly.
- `todo.md` records the final proof/inspection notes for the analysis-only
  slice.
