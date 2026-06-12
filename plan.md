# Phase E2 Prepared Lookup API Private Pass-Context Readiness

Status: Active
Source Idea: ideas/open/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md

## Purpose

Turn the Phase E2 source idea into an analysis-only execution route for deciding
which public prepared lookup APIs are ready to become private pass context, and
which must remain public or be deferred.

Goal: write the durable E2 readiness analysis and any accepted one-boundary
follow-up implementation ideas without deleting, privatizing, or renaming
prepared lookup APIs in this plan.

## Core Rule

E2 classifies readiness. It must not implement contractions, delete helpers,
hide aggregate `PreparedFunctionLookups`, move target/prepared policy into BIR,
or claim broad prepared retirement.

## Read First

- ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md
- docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md
- ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md
- docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md
- ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md
- ideas/closed/220_phase_e1_route_identity_helper_contraction.md
- docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md
- docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md
- src/backend/prealloc/module.hpp
- src/backend/prealloc/prepared_lookups.hpp
- current accepted baseline state and local hook review state

## Current Targets

Durable analysis output:

- docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md

Candidate surfaces:

- selected `call_plans` Route 6 call-use source adapter paths
- selected `memory_accesses` Route 3 memory/source adapter paths
- selected `edge_publications` and `edge_publication_source_producers` Route 4
  or Route 5 publication, edge, and join adapter paths
- selected Route 1/2/5/6/7 source-producer helper or printer/oracle rows
- identity-only `move_bundles` or `value_homes` audit candidates
- the proven E1 helpers:
  `find_prepared_control_flow_branch_target_labels(...)`
  and `find_prepared_fused_compare_operand_producer_facts(...)`

## Non-Goals

- Do not edit implementation files as part of this analysis plan.
- Do not delete, privatize, or rename prepared lookup APIs directly.
- Do not delete aggregate `PreparedFunctionLookups` or hide it behind a new
  facade while public consumers remain.
- Do not retire `PreparedBirModule`, open draft 155 / E5, or claim broad
  prepared retirement.
- Do not move target ABI, layout, register, stack, address, move, call, branch,
  or emission policy into BIR.
- Do not weaken diagnostics, supported-path status, baselines, expected
  strings, fallback behavior, printer/debug behavior, wrappers, or oracle names.

## Working Model

- BIR owns semantics.
- Prepared owns target policy, fallback, and oracle surfaces that are still
  publicly consumed.
- Completed E1 helper contractions prove only the selected identity helper
  paths under agreement with prepared fallback and target policy preserved.
- E2 must reason one public API boundary or one helper family at a time.

## Execution Rules

- Keep analysis findings in the E2 durable document first.
- Create follow-up implementation ideas only when the analysis names exactly
  one public prepared API boundary or helper family, the proven BIR/route
  semantic owner, remaining prepared fallback/policy/oracle, every public
  consumer, and proof required before privatization or deletion can be claimed.
- Classify each candidate as exactly one of the readiness or retention buckets
  from the source idea, unless the document explicitly explains why no
  classification applies.
- Treat facade renames, wrapper moves, construction reshuffles, baseline
  refreshes, expected-string rewrites, and unsupported downgrades as non-proof.
- If a candidate depends on printer/debug/oracle replacement, defer to E3.
- If a candidate depends on cross-target wrapper migration, defer to E4.
- If a candidate depends on more semantic helper contraction, defer to a named
  E1 follow-up.
- Because this plan is analysis-only, proof is document completeness,
  traceability to inputs, and lifecycle consistency rather than a compiler test
  pass, unless a later executor touches code.

## Step 1: Read Inputs and Anchor E2 Scope

Goal: establish the factual baseline from Phase E0, Phase E1, the ownership
readiness map, D2 consumer analysis, and current prepared lookup headers.

Primary targets:

- required input files listed above
- docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md

Actions:

- Inspect each required input and record the specific facts E2 may rely on.
- Confirm the two closed E1 helpers prove only their selected identity paths.
- Confirm aggregate lookup construction, public fallback/oracle APIs, E3,
  E4, Route 8, E5, and draft 155 remain outside E2 implementation scope.
- Create or update the E2 durable document with source links, scope,
  assumptions, and the candidate list.

Completion check:

- The E2 document exists and contains a scoped baseline section that prevents
  treating E1 helper closure as proof of whole lookup-group deletion.

## Step 2: Inventory Public Consumers by Candidate Surface

Goal: identify which production, printer/debug, target-wrapper, oracle,
fallback, and expected-string consumers still require each public prepared
surface.

Primary targets:

- src/backend/prealloc/module.hpp
- src/backend/prealloc/prepared_lookups.hpp
- docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md

Actions:

- Inventory consumers for each E2 candidate surface.
- Separate semantic identity consumers from prepared fallback, target policy,
  printer/debug, wrapper, and oracle consumers.
- Preserve exact helper/API names where the source inputs provide them.
- Record blockers without converting them into implementation work.

Completion check:

- The E2 document contains a candidate-by-candidate consumer inventory with
  public consumers and retained prepared responsibilities visible.

## Step 3: Classify Readiness One Boundary at a Time

Goal: decide whether each candidate is ready for a one-boundary implementation
idea, must remain public, or must defer to another phase or prerequisite.

Primary target:

- docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md

Actions:

- Classify each candidate using the source idea's readiness categories:
  ready to draft, private pass-context candidate after another named E1
  contraction, retained public fallback/oracle, retained target/prepared
  policy, E3 diagnostic/oracle prerequisite, E4 cross-target wrapper
  prerequisite, blocked aggregate compatibility surface, or no-action.
- Include explicit no-action decisions for aggregate lookup construction and
  target-policy/fallback/oracle surfaces.
- Include deferrals to E1, E3, E4, Route 8, or E5 where E2 is not the owner.
- State that draft 155 / E5 and aggregate `PreparedFunctionLookups` retirement
  remain unopened.

Completion check:

- The E2 document contains a readiness table covering every candidate surface
  named in the source idea, with no broad deletion or privatization claim.

## Step 4: Write Accepted Follow-Up Ideas

Goal: create durable implementation ideas only for candidates whose E2 analysis
supports one-boundary follow-up work.

Primary targets:

- ideas/open/*.md for accepted follow-up ideas
- docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md

Actions:

- For each accepted follow-up, create exactly one idea scoped to one public
  prepared API boundary or one helper family.
- Name the proven BIR/route semantic owner, retained prepared fallback,
  retained target policy or oracle, every public consumer still present, and
  required proof before privatization or deletion can be claimed.
- Include reviewer reject signals that block testcase-shaped shortcuts,
  unsupported downgrades, expectation rewrites, facade-only changes, broad
  rewrites, and retained old failure modes under new names.
- Link each accepted follow-up idea from the E2 durable document.

Completion check:

- Every accepted follow-up is represented either as a new scoped idea under
  `ideas/open/` or as an explicit no-action/deferred decision in the E2
  document; no implementation file has been changed.

## Step 5: Close Out E2 Analysis

Goal: make the E2 output reviewable and ready for lifecycle closure judgment.

Primary target:

- docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md

Actions:

- Verify the document includes the required linkable analysis payload,
  candidate readiness table, accepted follow-up ideas, explicit no-action
  decisions, deferrals, proof requirements, and unopened E5/draft 155 statement.
- Check that no implementation edits, expected-string rewrites, baseline
  refreshes, or unsupported downgrades were used as proof.
- Leave `todo.md` with the latest completed packet, proof, and any blockers for
  supervisor or plan-owner closure review.

Completion check:

- The source idea's expected output is fully represented in the E2 document and
  any accepted follow-up ideas, and the active lifecycle state is ready for
  closure review without claiming implementation progress.
