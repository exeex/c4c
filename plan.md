# Phase D Follow-Up Closure And Pre-Phase-E Readiness Audit Runbook

Status: Active
Source Idea: ideas/open/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md

## Purpose

Audit the completed Phase D follow-up route-view migrations and produce the
readiness record needed before the real Phase E `PreparedBirModule` retirement
analysis can be opened.

## Goal

Create `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`,
classify the closed follow-up ideas 182-189 as Phase D follow-ups, record the
remaining prepared fallback/oracle and target-policy boundaries, and create or
recommend prerequisite follow-up ideas before draft 155 is activated.

## Core Rule

This runbook is analysis-only. Do not edit backend implementation, contract or
delete prepared APIs, rename closed idea files for cosmetic phase cleanup, or
start the draft Phase E `PreparedBirModule` retirement plan.

## Read First

- `ideas/open/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md`
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
- `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md`
- closed return-chain follow-up ideas 176-180
- current accepted `test_baseline.log`

## Current Scope

- Durable audit artifact:
  `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- Closed implementation ideas 182-189 and their relationship to Phase D
  follow-up work.
- Route 1-7 selected-consumer migration coverage plus the cross-target reuse
  slice.
- Prepared fallback/oracle, printer/debug, diagnostics, and aggregate surfaces
  still relevant to `PreparedFunctionLookups` or `PreparedBirModule`
  retirement.
- Target-policy boundaries that must remain prepared-owned or target-owned.
- Follow-up ideas under `ideas/open/` for separate prerequisite initiatives
  found by the audit.

## Non-Goals

- Do not edit AArch64, x86, riscv, BIR, or prealloc implementation code.
- Do not contract, delete, privatize, or rename prepared APIs.
- Do not activate or execute
  `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`.
- Do not claim route-wide or `PreparedBirModule` field-wide readiness from one
  selected-consumer migration.
- Do not treat a green full-suite baseline as proof that prepared
  fallback/oracle surfaces can be deleted.
- Do not fold return-chain ownership into generic Route 1, Route 7, or
  aggregate retirement analysis without importing the dedicated owner/schema
  line from ideas 176-180.

## Working Model

Ideas 182-189 are Phase D follow-up implementation slices with Phase E-like
filenames, not the real Phase E retirement analysis. Each selected-consumer
migration proves a bounded route-view consumer boundary and leaves prepared
fallback/oracle or target-policy surfaces to audit before retirement planning.
Idea 190 adds a specific readiness rule: Route 3 semantic memory/source
identity must not replace AArch64 prepared target-addressing or materialization
policy.

## Execution Rules

- Keep routine audit progress and evidence notes in `todo.md`.
- Preserve source idea 191 as durable intent unless closure or a real source
  intent correction requires a minimal update.
- Write the durable audit as a docs artifact, not as scattered chat or
  `todo.md` notes.
- When creating prerequisite ideas, keep each idea scoped to a separate
  initiative and include concrete reviewer reject signals.
- Do not weaken baseline, c_testsuite, route-debug, wrapper, oracle, or target
  emission contracts to make retirement readiness appear closer.
- Treat `PreparedBirModule` retirement readiness as blocked unless the audit
  can name the residual prepared surfaces, fallback/oracle owners, target
  policy boundaries, and prerequisite follow-up ideas.

## Ordered Steps

### Step 1: Inventory Source Evidence

Goal: build the audit evidence set before drawing readiness conclusions.

Primary targets:
- source idea 191
- required docs, closed ideas 176-190, draft 155, and `test_baseline.log`

Actions:
- Read the Phase D switch plan, closed Phase D follow-up ideas 181-190, draft
  155, the return-chain owner/schema decision, and return-chain ideas 176-180.
- Confirm whether `test_baseline.log` is the current accepted full-suite
  baseline and extract the relevant proof summary.
- Record in `todo.md` any missing source material, ambiguous close note, or
  evidence gap before writing readiness conclusions.
- Do not edit the durable audit artifact until the required evidence inventory
  is complete enough to support the tables.

Completion check:
- `todo.md` lists the evidence read, any missing material, and the accepted
  baseline evidence that the audit will cite.

### Step 2: Build Route And Boundary Tables

Goal: classify what each selected migration proved and what remains prepared
owned.

Primary targets:
- closed ideas 182-189
- route-view migration coverage by route
- prepared fallback/oracle and target-policy surfaces

Actions:
- For each idea 182-189, record the selected consumer, BIR route view or facade
  used, prepared fallback/oracle retained, target-policy boundary preserved,
  proof scope, and known residual prepared readers.
- Build readiness tables for route coverage, remaining public prepared
  fallback/oracle surfaces, target-policy surfaces that stay prepared-owned or
  target-owned, x86/riscv cross-target reuse status, and return-chain import
  status.
- Incorporate idea 190's Route 3 finding as a blocker or readiness rule, not
  merely as a fixed test failure.
- Identify residual consumers that still block `PreparedFunctionLookups` or
  `PreparedBirModule` retirement analysis.

Completion check:
- `todo.md` contains the table outline or extracted facts needed to draft the
  durable audit without re-reading every source file.

### Step 3: Write The Durable Readiness Audit

Goal: create the durable audit artifact with explicit readiness conclusions.

Primary target:
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`

Actions:
- Write the audit artifact with the route coverage, prepared fallback/oracle,
  target-policy, cross-target reuse, return-chain, and retirement blocker
  tables.
- State explicitly that ideas 182-189 are Phase D follow-up slices, not the
  true Phase E retirement plan.
- State whether draft 155 is ready to open immediately and why.
- List every follow-up idea created or recommended by the audit.
- Avoid claiming prepared API deletion, broad route migration, or
  `PreparedBirModule` retirement from selected-consumer evidence alone.

Completion check:
- The docs artifact exists, contains the required tables and readiness
  conclusion, and names follow-up idea requirements.

### Step 4: Create Prerequisite Follow-Up Ideas

Goal: make every separate prerequisite initiative durable under `ideas/open/`.

Primary targets:
- `ideas/open/*.md` follow-up ideas required by the audit
- durable audit artifact's follow-up list

Actions:
- Create separate open ideas for prerequisite initiatives the audit identifies,
  such as residual route-view consumer migrations, Route 3 boundary hardening,
  prepared printer/debug/oracle replacement planning, cross-target reuse beyond
  the single Route 6 boundary, `PreparedFunctionLookups` readiness, lifecycle
  naming clarification, or other blockers to draft 155.
- Include goal, scope, out-of-scope work, acceptance criteria, and concrete
  reviewer reject signals in each new idea.
- Keep audit-only commentary in the docs artifact and follow-up intent in the
  new source idea files.
- Do not create broad catch-all ideas when the audit identifies separable
  initiatives.

Completion check:
- Every prerequisite the audit says must precede true Phase E retirement is
  represented by an open idea or explicitly justified as recommended but not
  created.

### Step 5: Final Audit Consistency Check

Goal: make the lifecycle slice ready for supervisor review and closure
decision.

Primary targets:
- durable audit artifact
- new follow-up ideas under `ideas/open/`
- `todo.md` proof and summary notes

Actions:
- Check the audit against source idea 191 acceptance criteria and reviewer
  reject signals.
- Check that no implementation files were modified.
- Check that the audit does not open or execute draft 155 and does not present
  selected-consumer migrations as route-wide retirement readiness.
- Record the final file list, docs/output summary, follow-up ideas created,
  and lifecycle recommendation in `todo.md`.
- Ask the supervisor to decide whether to close idea 191 after any required
  docs-only validation or review.

Completion check:
- `todo.md` records the final audit proof, follow-up idea list, and closure
  recommendation; source idea 191 can be evaluated for closure without
  re-deriving the audit.
