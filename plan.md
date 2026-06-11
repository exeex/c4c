# Phase D MIR Consumer BIR View Switch Runbook

Status: Active
Source Idea: ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md

## Purpose

Turn the remaining MIR/codegen dependencies on `Prepared*` query surfaces into a
durable Phase D consumer-switch analysis artifact and bounded follow-up ideas.

Goal: produce `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
without converting backend lowering or deleting prepared APIs.

Core Rule: Phase D is analysis-only. Do not rewrite AArch64, x86, or riscv
lowering, and do not claim prepared public surface contraction from selected
consumer migrations alone.

## Read First

- `ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `ideas/closed/167_route1_producer_constant_oracle_thinning.md`
- `ideas/closed/168_route2_select_chain_direct_global_oracle_thinning.md`
- `ideas/closed/169_route3_semantic_memory_access_cache_split.md`
- `ideas/closed/170_route4_block_entry_publication_migration.md`
- `ideas/closed/171_route5_current_block_join_source_migration.md`
- `ideas/closed/172_route6_call_use_semantic_source_migration.md`
- `ideas/closed/173_route7_comparison_condition_oracle_thinning.md`
- `ideas/closed/174_route_index_facade_contraction.md`

## Current Targets

- Durable analysis artifact:
  `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- Consumer dependency map for MIR/codegen reads of `PreparedBirModule`,
  `PreparedFunctionLookups`, and domain `Prepared*` query structs.
- Ordered migration ladder for route-backed consumer groups.
- Temporary adapter and oracle boundaries.
- Bounded follow-up implementation ideas for safe consumer migrations.
- Explicit no-route blockers, including return-chain ownership/schema.

## Non-Goals

- Do not directly convert backend lowering consumers.
- Do not delete or hide prepared APIs during this phase.
- Do not move target/layout/codegen policy into BIR.
- Do not treat `RouteIndexReferenceFacade` as a generic
  `PreparedFunctionLookups` replacement.
- Do not fold return-chain lookup ownership into Route 1, Route 7, or generic
  Phase D work.

## Working Model

Classify each prepared dependency as exactly one of:

- route-backed semantic fact ready for a bounded BIR-view migration idea
- temporary migration oracle that must remain public until equivalence is proven
- durable target/layout/codegen policy surface that stays prepared-owned or
  target-owned
- no-route gap requiring a separate owner/schema decision before migration

Preserve prepared answers as oracles until equivalence is proven. Proposed BIR
views should expose annotated node or route facts, not target policy.

## Execution Rules

- Keep source-idea intent stable; routine findings belong in the Phase D doc or
  `todo.md`.
- Use repo search and existing docs to map consumers; do not infer readiness
  from a single selected migration.
- Prefer bounded follow-up ideas per semantic consumer group.
- Name the prepared oracle/fallback boundary for every proposed migration.
- For every code-changing follow-up idea, include a proof ladder:
  build -> oracle equivalence -> narrow backend subset -> broader check when
  blast radius crosses consumer groups.
- Treat testcase-shaped shortcuts, expectation downgrades, and AArch64-only
  convenience APIs as reject signals.

## Ordered Steps

### Step 1: Gather Phase A-C and Route Closure Evidence

Goal: establish the factual baseline for Phase D.

Primary targets:

- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `ideas/closed/167_route1_producer_constant_oracle_thinning.md`
- `ideas/closed/168_route2_select_chain_direct_global_oracle_thinning.md`
- `ideas/closed/169_route3_semantic_memory_access_cache_split.md`
- `ideas/closed/170_route4_block_entry_publication_migration.md`
- `ideas/closed/171_route5_current_block_join_source_migration.md`
- `ideas/closed/172_route6_call_use_semantic_source_migration.md`
- `ideas/closed/173_route7_comparison_condition_oracle_thinning.md`
- `ideas/closed/174_route_index_facade_contraction.md`

Actions:

- Extract the residual prepared-consumer findings from Phase C and routes
  167-174.
- Record which route records, typed indexes, or facades are already available.
- Record which route families remain oracle-only or selected-consumer-only.
- Keep return-chain lookup as a no-route gap unless a separate owner/schema
  source exists.

Completion check:

- `todo.md` or the draft artifact contains a concise evidence table that
  preserves the Phase C residual-consumer findings without upgrading them into
  API contraction claims.

### Step 2: Map Prepared Consumer Families

Goal: identify which MIR/codegen consumers still depend on prepared surfaces.

Primary targets:

- MIR/codegen consumer files discovered by searches for `PreparedBirModule`,
  `PreparedFunctionLookups`, and domain `Prepared*` query structs.
- AArch64 traversal/dispatch, calls, memory/addressing, value materialization
  and publication, comparison/ALU, edge copies/control flow, wide values,
  runtime helpers, and future x86/riscv interfaces.

Actions:

- For each consumer family, name the prepared API or helper it reads.
- Classify the dependency using the four-category working model.
- Identify whether an existing BIR route annotation, typed route index, or
  narrow facade can support a migration.
- Separate durable target/layout/codegen policy from semantic facts that can
  live in BIR views.

Completion check:

- The Phase D artifact has a consumer-family dependency map from prepared APIs
  to proposed BIR view/adaptor surfaces or blocker classifications.

### Step 3: Define BIR View and Adapter Boundaries

Goal: propose consumer-facing BIR view APIs that are narrow enough to migrate
one semantic group at a time.

Actions:

- For route-backed groups, describe the BIR annotation view or adapter that
  should replace the prepared read.
- For oracle-only groups, name the prepared fallback that must remain public.
- For target-policy groups, explicitly keep ownership prepared-side or
  target-side.
- For no-route gaps, describe the missing owner/schema decision and avoid
  inventing scans, predecessor rescans, or name matching.

Completion check:

- The artifact names temporary adapter boundaries and distinguishes semantic
  BIR facts from target/codegen policy for each mapped family.

### Step 4: Order the Migration Ladder

Goal: convert the dependency map into a safe sequence of bounded follow-up work.

Actions:

- Order route-backed consumer migrations from lowest blast radius to highest.
- Put oracle-only and no-route gaps behind their required Phase A/B/C
  prerequisites.
- For each proposed follow-up, name the consumer group, BIR prerequisite,
  prepared oracle/fallback boundary, out-of-scope target policy, and proof
  recommendation.
- Keep each follow-up small enough to become a separate implementation idea.

Completion check:

- The artifact contains an ordered migration ladder and bounded follow-up idea
  summaries that Phase E can consume directly.

### Step 5: Finalize Phase D Artifact and Lifecycle Notes

Goal: make the analysis artifact ready for review and closure.

Actions:

- Write or update
  `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.
- Ensure it links back to the Phase A-C documents and route closure notes.
- Ensure it restates the Phase C residual-consumer findings accurately.
- Ensure return-chain is excluded with a separate owner/schema need or marked
  as a no-route blocker.
- Do not create implementation diffs as part of this phase.

Completion check:

- The Phase D artifact exists and satisfies the source idea completion
  criteria.
- `todo.md` records the latest proof or review result for the artifact-only
  change.
