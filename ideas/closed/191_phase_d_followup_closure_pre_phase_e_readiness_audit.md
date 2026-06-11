# 191 Phase D follow-up closure and pre-Phase-E readiness audit

## Goal

Audit the completed Phase D follow-up implementation ideas and produce the
next set of source ideas required before the real Phase E
`PreparedBirModule` retirement analysis can begin.

This is analysis-only. It must not directly modify backend implementation,
contract prepared APIs, or start `PreparedBirModule` retirement work.

## Why This Exists

Phase D analysis closed in
`ideas/closed/181_phase_d_mir_consumer_bir_view_switch_plan.md` and produced a
one-rung consumer migration ladder. Ideas 182-189 executed selected follow-up
migrations from that ladder, but their filenames use `phase_e_*` even though
they are not the Phase E retirement analysis described by
`ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`.

The baseline repair in
`ideas/closed/190_full_suite_baseline_99_percent_regression_attribution.md`
also proved that at least one route-view migration exposed a real design
boundary: Route 3 semantic memory/source identity cannot replace AArch64
prepared target-addressing policy, and selected route-first reads must preserve
prepared fallback where target policy remains prepared-owned.

Before activating the real Phase E retirement plan, the project needs a durable
readiness audit that separates:

- completed selected-consumer route-view migrations;
- residual prepared fallback/oracle surfaces;
- target-policy surfaces that must stay prepared-owned or target-owned;
- naming/lifecycle cleanup needed because 182-189 are Phase D follow-ups, not
  true Phase E retirement work;
- follow-up ideas needed before `PreparedBirModule` retirement analysis can
  safely reason field-by-field.

## Source Material

This audit must read:

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

## Durable Output

Write the audit payload to:

- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`

The closure note must summarize that artifact and list every follow-up idea
created or recommended.

## In Scope

- Reclassify ideas 182-189 as Phase D follow-up implementation slices, not
  true Phase E retirement work.
- For each route 1-7 and the cross-target reuse slice, summarize what selected
  consumer boundary was proven and what prepared fallback/oracle surfaces
  remain.
- Incorporate idea 190's Route 3 regression finding into the readiness rules:
  semantic route views must not replace prepared target-addressing or
  materialization policy.
- Identify which residual consumers still block `PreparedFunctionLookups` or
  `PreparedBirModule` retirement analysis.
- Identify target-policy families that should remain out of BIR during any
  future retirement plan.
- Decide whether the real Phase E retirement draft can be opened immediately
  or whether more prerequisite follow-up ideas are required.
- Produce those follow-up ideas under `ideas/open/` when the audit finds
  separate initiatives.

## Out Of Scope

- Directly editing AArch64, x86, riscv, BIR, or prealloc implementation code.
- Contracting, deleting, or privatizing prepared APIs.
- Renaming closed idea files only for cosmetic phase-label cleanup.
- Activating or executing
  `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`.
- Treating one selected consumer migration as route-wide readiness for
  retirement.
- Treating a green full-suite baseline as proof that prepared fallback/oracle
  surfaces can now be deleted.

## Required Analysis

For each closed follow-up idea 182-189, record:

- selected consumer migrated;
- BIR route view or facade used;
- prepared fallback/oracle retained;
- target-policy boundary preserved;
- proof scope and whether full-suite baseline evidence exists;
- residual consumers or diagnostics that still read prepared surfaces.

Then produce readiness tables for:

- route-view migration coverage by route;
- prepared fallback/oracle surfaces that remain public;
- target-policy surfaces that must remain prepared-owned or target-owned;
- cross-target reuse status for x86 and riscv;
- return-chain status and whether closed ideas 176-180 are importable by the
  future Phase E retirement analysis;
- blockers to a field-by-field `PreparedBirModule` retirement map.

## Expected Follow-Up Ideas

The audit should create or recommend follow-up ideas when it finds work that is
separate from the audit itself. Likely categories include:

- residual route-view consumer migrations for routes where only one selected
  consumer moved;
- Route 3 fallback/design-boundary hardening after the baseline regression;
- prepared printer/debug/oracle replacement planning before API contraction;
- x86 or riscv route-view reuse beyond the single Route 6 `ConsumedPlans`
  boundary;
- `PreparedFunctionLookups` aggregate ownership/readiness audit after the
  selected migrations;
- a lifecycle/naming clarification note that distinguishes Phase D follow-ups
  from the real Phase E retirement plan;
- prerequisites that must be complete before opening the draft Phase E
  `PreparedBirModule` retirement analysis.

## Acceptance Criteria

- The durable audit artifact exists at
  `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`.
- The artifact explicitly states whether draft 155 is ready to open now, and
  why.
- The artifact does not claim prepared API deletion, broad route migration, or
  `PreparedBirModule` retirement from selected-consumer evidence alone.
- Idea 190's first-bad Route 3 finding is reflected as a readiness rule or
  blocker.
- Follow-up ideas are created under `ideas/open/` for every separate initiative
  the audit says must happen before true Phase E retirement analysis.
- No implementation source changes are required for this analysis.

## Reviewer Reject Signals

- The audit treats ideas 182-189 as the true Phase E retirement plan.
- The audit opens or executes draft 155 without first resolving the listed
  readiness questions.
- A selected consumer migration is presented as route-wide or
  `PreparedBirModule` field-wide readiness.
- The Route 3 baseline regression is ignored or summarized only as a fixed
  test failure rather than a semantic/prepared-policy boundary lesson.
- Prepared printer/debug/oracle consumers are omitted from the readiness
  analysis.
- Return-chain is folded into Route 1, Route 7, or generic aggregate
  retirement without importing the dedicated return-chain owner/schema line.
- The audit weakens test or baseline requirements to make retirement look
  ready.

## Closure Note

Closed after the active runbook completed Step 5. The durable audit artifact is
`docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`.

The audit concludes that draft 155 is not ready to open immediately, treats
ideas 182-189 as Phase D follow-up implementation slices rather than the true
Phase E retirement plan, records idea 190's Route 3 prepared-policy boundary as
a readiness rule, and avoids claiming prepared API deletion, route-wide
migration, or `PreparedBirModule` retirement from selected-consumer evidence.

The audit created separate prerequisite follow-up ideas:

- `ideas/open/192_residual_route_view_consumer_migration_map.md`
- `ideas/open/193_route3_prepared_policy_boundary_hardening.md`
- `ideas/open/194_prepared_printer_debug_oracle_replacement_planning.md`
- `ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md`
- `ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md`
- `ideas/open/197_return_chain_import_and_naming_clarification.md`
- `ideas/open/198_phase_d_phase_e_lifecycle_naming_cleanup.md`

Close-time regression guard used existing matching backend logs:
`test_before.log` and `test_after.log` both passed 180 of 180 tests with no new
failures. The lifecycle-only close was accepted with the documented
non-decreasing guard mode.
