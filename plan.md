# Phase A2 Residual Prepared Surface Semantic-Owner Audit Runbook

Status: Active
Source Idea: ideas/open/200_phase_a2_residual_prepared_surface_semantic_owner_audit.md

## Purpose

Audit residual prepared/prealloc surfaces that are not already owned by Routes
1-8, then classify each surface by durable semantic owner before any schema,
consumer-switch, contraction, or `PreparedBirModule` retirement work.

## Goal

Produce `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
with evidence-backed ownership classifications and follow-up idea payloads.

## Core Rule

This runbook is analysis-only. Do not add BIR records, migrate consumers,
contract prepared APIs, weaken expectations, or treat classification as
retirement readiness.

## Read First

- `ideas/open/200_phase_a2_residual_prepared_surface_semantic_owner_audit.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_b_annotation_schema_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
- `ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`
- `ideas/closed/199_full_suite_baseline_string_authority_timeout_attribution.md`
- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`

## Current Targets

- Durable output: `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
- Primary prepared aggregate surfaces:
  - `src/backend/prealloc/module.hpp`
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/prepared_lookups.cpp`
- Cross-target wrapper and consumer surfaces under:
  - `src/backend/mir/aarch64/`
  - `src/backend/mir/x86/`
  - `src/backend/mir/riscv/`
- Prepared printer, debug, dump, helper, oracle, and baseline surfaces that
  keep prepared APIs alive.

## Non-Goals

- Do not implement new route schemas or BIR indexes.
- Do not migrate MIR/codegen consumers.
- Do not delete, privatize, or contract prepared APIs.
- Do not open or execute draft 155 retirement work.
- Do not force target policy, ABI/layout, diagnostics, or pass context into BIR.
- Do not downgrade tests, baselines, unsupported markers, or string-authority
  rules to make ownership look cleaner.

## Working Model

Classify each residual surface into exactly one primary owner category, with
notes for mixed cases:

- new target-neutral BIR route candidate
- existing Route 1-8 extension candidate
- target/prepared policy retained outside BIR
- transient pass context or private construction cache
- diagnostic/printer/oracle compatibility surface
- blocked or unknown surface requiring deeper source inspection

For each row, capture current files and APIs, current consumers, semantic
ownership rationale, route coverage or retention decision, proof evidence, and
any required follow-up idea payload.

## Execution Rules

- Prefer source evidence over naming intuition; inspect readers before
  classifying a surface.
- Preserve the idea 190 and idea 199 baseline lessons in every classification
  that touches fallback, target policy, or string authority.
- Include AArch64, x86, and riscv wrapper surfaces when they hold prepared
  contracts alive.
- Keep printer/debug/oracle compatibility separate from production lowering.
- Follow-up ideas may be recorded as payloads inside the audit artifact; create
  new files under `ideas/open/` only if the supervisor delegates lifecycle
  filing.
- For this analysis-only runbook, proof is artifact completeness plus cited
  source evidence. Code build proof is not required unless execution edits code.

## Ordered Steps

### Step 1: Inventory Inputs And Create Audit Skeleton

Goal: establish the evidence base and artifact shape before classification.

Primary target: `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`

Actions:

- Read every document and idea listed in `Read First`.
- Create the Phase A2 audit artifact if it does not exist.
- Add sections for evidence inventory, residual ownership table, accepted route
  candidates, rejected retained surfaces, blocked unknowns, follow-up payloads,
  and closure notes.
- Seed the table with residual surface groups named by the source idea and prior
  artifacts, without final classification until source readers are inspected.

Completion check:

- The audit artifact exists and names the source idea.
- Every required input is either cited as read or explicitly marked missing with
  the impact on classification.
- The residual table has a placeholder row for each known prepared/prealloc
  surface group that requires audit.

### Step 2: Classify Prepared Aggregate And Lookup Surfaces

Goal: classify `PreparedBirModule`, `PreparedFunctionLookups`, and retained or
mixed lookup groups by real ownership.

Primary targets:

- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- related lookup owners referenced from the Phase A-D artifacts

Actions:

- Inspect `PreparedBirModule` fields and wrapper state not already mapped to
  Routes 1-8.
- Inspect `PreparedFunctionLookups` groups classified as retained or mixed in
  idea 196 and its durable map.
- For each group, record API names, current consumers, route coverage, retained
  target/prepared policy, pass-context/cache status, and diagnostic/oracle
  obligations.
- Reject whole-aggregate BIR cloning, facade renames, and deletion readiness
  unless one real semantic consumer boundary and fallback oracle are proven.

Completion check:

- The audit table classifies every aggregate and lookup group with cited file or
  prior-artifact evidence.
- Mixed groups identify the exact route-native subfact and the exact retained
  prepared policy.
- No row claims contraction or retirement readiness from classification alone.

### Step 3: Classify Cross-Target Wrappers And Diagnostic Oracles

Goal: account for target wrapper, printer, debug, dump, helper, and oracle
surfaces that preserve prepared ownership.

Primary targets:

- AArch64 prepared wrapper and consumer files under `src/backend/mir/aarch64/`
- x86 prepared wrapper and consumer files under `src/backend/mir/x86/`
- riscv prepared wrapper and consumer files under `src/backend/mir/riscv/`
- prepared printer, route-debug, lookup-helper, and oracle tests or helpers

Actions:

- Inspect target-specific wrapper readers for AArch64, x86, and riscv.
- Separate production-lowering consumers from diagnostic/printer/oracle
  compatibility consumers.
- Identify surfaces whose only safe near-term owner is retained target policy,
  retained oracle compatibility, transient pass context, or blocked/unknown.
- Apply idea 190 and 199 lessons where prepared fallback, target policy, full
  baseline behavior, or string-authority behavior could regress.

Completion check:

- The audit artifact includes cross-target coverage, including x86 and riscv
  when they keep prepared APIs public.
- Diagnostic/oracle rows name what equivalence proof would be needed before
  replacement.
- No classification omits a retained prepared surface because it is not on the
  AArch64 production path.

### Step 4: Produce Ownership Decisions And Follow-Up Payloads

Goal: turn classifications into actionable route candidates, retained-surface
decisions, blocked items, and future idea payloads.

Primary target: `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`

Actions:

- Fill the residual ownership table with final classifications and rationale.
- List accepted new route candidates and existing Route 1-8 extension
  candidates, if any.
- List rejected target-policy, pass-context/cache, diagnostic/oracle, and
  retained prepared surfaces with concrete rationale.
- List blocked or unknown surfaces and the exact source inspection required to
  unblock them.
- Draft follow-up idea payloads for Phase B2 schema work, retained-policy
  cleanup, diagnostic/oracle replacement, or focused blocked-surface audits.

Completion check:

- Each accepted candidate names the semantic fact, owner category, excluded
  target/prepared policy, proof route, and reject signals.
- Each rejected retained surface names why it must stay outside BIR.
- Follow-up payloads are specific enough for a plan owner to file as separate
  `ideas/open/` records later without re-deriving the audit.

### Step 5: Closure Handoff

Goal: leave a clear closure-ready analysis artifact and lifecycle handoff.

Primary target: `todo.md`

Actions:

- Verify the audit artifact contains the required output from the source idea:
  link, residual ownership table, accepted candidates, rejected retained
  surfaces, blocked unknowns, and follow-up ideas.
- Record latest progress, proof, and any blockers in `todo.md`.
- If the source idea appears complete, ask the supervisor to route a plan-owner
  close decision instead of closing directly from executor work.

Completion check:

- `todo.md` identifies the completed plan step and proof evidence.
- The source idea can be judged against the expected output without relying on
  chat history.
- Any remaining unknowns are recorded as blocked classifications or follow-up
  idea payloads, not silent omissions.
