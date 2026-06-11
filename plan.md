# Residual Route-View Consumer Migration Map Runbook

Status: Active
Source Idea: ideas/open/192_residual_route_view_consumer_migration_map.md

## Purpose

Produce the route-by-route residual prepared consumer map needed before true
Phase E `PreparedBirModule` retirement analysis can reason about public
prepared API contraction.

## Goal

Create a durable route-family migration map that names remaining prepared
consumers for Routes 1-7, classifies each consumer type, and identifies the
next selected-consumer migrations or retained-policy reasons for each route.

## Core Rule

This runbook is analysis-only. Do not implement migrations, delete or
privatize prepared APIs, weaken oracle or diagnostic expectations, combine
unrelated route families into one implementation slice, or open draft 155.

## Read First

- `ideas/open/192_residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- `ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md`
- `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`
- `ideas/closed/183_phase_e_route5_edge_join_source_view_consumer_migration.md`
- `ideas/closed/184_phase_e_route1_producer_constant_view_consumer_migration.md`
- `ideas/closed/185_phase_e_route2_select_chain_view_consumer_migration.md`
- `ideas/closed/186_phase_e_route3_memory_semantic_view_consumer_migration.md`
- `ideas/closed/187_phase_e_route6_call_use_source_view_consumer_migration.md`
- `ideas/closed/188_phase_e_route7_comparison_view_consumer_migration.md`
- `ideas/closed/189_phase_e_cross_target_route_view_interface_reuse.md`
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`
- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`

## Current Scope

- Residual prepared consumers for Routes 1-7.
- Production lowering, printer/debug, target-wrapper, oracle/test, and
  retained target/prepared policy consumers.
- Already migrated selected readers from ideas 182-188.
- Candidate next selected-consumer migrations and route-view or facade facts
  required by those candidates.
- Proof expectations for future migrations, including nearby same-feature
  coverage that must remain represented.
- Durable output artifact:
  `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`.

## Non-Goals

- Do not edit backend implementation files.
- Do not implement any migration named by the map.
- Do not delete, privatize, rename, or contract prepared APIs.
- Do not rewrite route-view contracts unless the map proves a specific future
  idea should own that contract gap.
- Do not combine Routes 1-7 into a catch-all implementation plan.
- Do not open or execute draft 155.
- Do not treat selected-consumer evidence from ideas 182-188 as route-wide
  completion.

## Working Model

Ideas 182-188 prove one selected route-view consumer per route family. That is
bounded prior art, not route-wide consumer exhaustion. The map must identify
which prepared readers remain, which can move next, which readers are retained
as fallback or oracle surfaces, and which route-view facts are missing before a
future implementation idea can safely migrate a selected consumer.

## Execution Rules

- Keep routine evidence notes and unresolved questions in `todo.md`.
- Preserve source idea 192 unless durable intent changes.
- Write the migration map as a docs artifact, not as chat-only notes.
- Classify residual consumers by actual role, not by desired retirement state.
- Keep target policy, ABI policy, address-materialization policy, and final
  emission policy outside route-view scope.
- Treat printer/debug and oracle consumers as first-class blockers instead of
  hiding them behind production-only readiness.
- For each future migration candidate, name proof that would reject a
  testcase-shaped shortcut.

## Ordered Steps

### Step 1: Inventory Prior Route Evidence

Goal: establish the selected-consumer baseline before classifying residual
readers.

Primary targets:
- source idea 192
- Phase D switch and readiness docs
- closed ideas 181-190
- draft 155 for retirement-analysis expectations

Actions:
- Read the required source idea, docs, closed ideas, and draft 155.
- Record in `todo.md` the selected reader moved by each of ideas 182-188, the
  route-view or facade it used, retained prepared fallback/oracle surfaces,
  target-policy boundaries, and proof scope.
- Treat idea 189 as cross-target evidence, not as proof that every target
  wrapper can reuse every route view.
- Record missing or ambiguous source material before drawing route-wide
  conclusions.

Completion check:
- `todo.md` lists the evidence baseline for Routes 1-7 and the selected
  readers already migrated.

### Step 2: Locate Residual Prepared Consumers

Goal: identify remaining prepared readers by route family and consumer role.

Primary targets:
- `PreparedBirModule`
- `PreparedFunctionLookups`
- route-specific prepared helper accessors
- printer/debug, target-wrapper, oracle, and c_testsuite-facing consumers

Actions:
- Inspect current code and tests only enough to inventory residual readers;
  avoid implementation edits.
- For Routes 1-7, classify residual consumers as production lowering,
  printer/debug, target-wrapper, oracle/test, or retained target/prepared
  policy.
- Distinguish migrated selected readers from unmigrated residual readers.
- Preserve explicit unknowns when a consumer cannot be classified without a
  separate implementation investigation.

Completion check:
- `todo.md` contains a route-family residual-consumer table outline with
  classifications, migrated readers, residual readers, and unknowns.

### Step 3: Identify Next Migration Candidates

Goal: turn the inventory into separable future implementation candidates.

Primary targets:
- route-family residual-consumer table from Step 2
- route-view or facade facts needed by candidate migrations
- fallback/oracle and target-policy boundaries

Actions:
- For each route, identify one or more next selected-consumer migrations, or
  give an explicit reason no migration should precede Phase E.
- Name the route-view facts, facade facts, or compatibility adapters each
  candidate would need.
- Name fallback behavior for absent, mismatched, ambiguous, or
  policy-sensitive facts.
- State proof expectations, including nearby same-feature tests and negative
  fallback coverage where relevant.

Completion check:
- Each route family has a concrete next-migration proposal or a durable
  retained-policy reason.

### Step 4: Write The Durable Migration Map

Goal: create the durable route-family map that future plan owners can use
without re-deriving the inventory.

Primary target:
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`

Actions:
- Write the map with a table for Routes 1-7 covering selected-reader evidence,
  residual prepared consumers, consumer classification, candidate migrations,
  required route facts, fallback/oracle surfaces, and proof expectations.
- Include a separate section for printer/debug and oracle consumers that block
  prepared API contraction.
- Include a separate section for target-wrapper and retained policy consumers.
- Name future implementation ideas only at proposal granularity; do not turn
  the map into a broad implementation plan.

Completion check:
- The docs artifact exists and satisfies the source idea acceptance criteria
  for residual consumer classification and next selected-consumer proposals.

### Step 5: Final Consistency Check

Goal: make the analysis slice ready for supervisor review and lifecycle
closure judgment.

Primary targets:
- durable migration map artifact
- source idea 192 acceptance criteria and reviewer reject signals
- `todo.md` final summary

Actions:
- Check that no implementation files changed.
- Check that the map does not claim route-wide completion from selected-reader
  evidence.
- Check that printer/debug, oracle, target-wrapper, and retained policy
  consumers are represented.
- Check that future candidates are separable by route family and do not
  weaken tests or expectations.
- Record the final artifact path, summary, open questions, and suggested
  lifecycle disposition in `todo.md`.

Completion check:
- `todo.md` records final proof and a closure recommendation for idea 192.
