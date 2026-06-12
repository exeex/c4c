# Phase E1 Route Identity Helper Contraction Runbook

Status: Active
Source Idea: ideas/open/220_phase_e1_route_identity_helper_contraction.md

## Purpose

Move one selected Route 1 through Route 7 source, publication, join, call, or
comparison identity helper or reader to route/prepared agreement while keeping
prepared ownership for target policy, fallback/oracle behavior, wrappers,
diagnostics, printer/debug rows, and expected strings.

## Goal

Replace or contract one duplicate semantic route identity read for one route
family and one selected consumer/helper, with proof that prepared fallback,
target policy, output, wrapper, oracle, and string behavior remain unchanged.

## Core Rule

Choose exactly one route family and one selected consumer or identity helper
before code changes. Do not open broad Route 1 through Route 7 migration,
generic route-index facade work, aggregate route-view replacement, prepared
aggregate retirement, or expectation rewrites.

## Read First

- `ideas/open/220_phase_e1_route_identity_helper_contraction.md`
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- route-specific closure notes and tests for the selected candidate only
- implementation and tests for the selected helper or reader only

## Current Targets

- exactly one route family from Route 1 through Route 7
- exactly one selected consumer or identity helper
- candidate surfaces:
  - `find_prepared_same_block_scalar_producer(...)`
  - `evaluate_prepared_same_block_integer_constant(...)`
  - `find_prepared_direct_global_select_chain_dependency(...)`
  - `find_prepared_store_source_direct_global_select_chain_dependency(...)`
  - `PreparedMemoryAccessLookups`
  - `find_prepared_memory_access(...)`
  - `find_prepared_global_load_access(...)`
  - `find_prepared_same_block_global_load_access(...)`
  - `PreparedEdgePublicationLookups`
  - `PreparedEdgePublicationSourceProducerLookups`
  - `find_prepared_current_block_entry_publication(...)`
  - `find_indexed_prepared_edge_publications(...)`
  - `find_indexed_prepared_edge_publication_source_producer(...)`
  - `PreparedCallPlanLookups`
  - `find_prepared_call_argument_source_producer_materialization(...)`
  - `find_prepared_call_result_late_publication(...)`
  - `find_prepared_fused_compare_operand_producer_facts(...)`
  - `find_prepared_materialized_condition_producer(...)`

## Non-Goals

- Do not migrate Route 1 through Route 7 broadly.
- Do not create generic route-index facades or aggregate route view
  replacement.
- Do not retire `PreparedFunctionLookups`, `PreparedBirModule`, call-plan,
  edge-publication, memory-access, move-bundle, publication, comparison,
  wrapper, or prepared APIs.
- Do not move address formation, value homes, storage, move scheduling, call
  ABI, publication construction, branch spelling, fused legality, target
  policy, wrappers, printer/debug rows, helper oracles, failure-mode fallback,
  or emitted output into route authority.
- Do not perform row-scoped diagnostic/oracle replacement unless the row is
  only the proof harness for the selected semantic identity reader.
- Do not perform liveness, intrinsic metadata, aggregate forwarding, E2, E3,
  E4, Route 8, draft 155, or E5 work.
- Do not use baseline refreshes, unsupported downgrades, helper renames,
  timeout masking, or expectation rewrites as proof.

## Working Model

Route facts may own only the selected identity fact under route/prepared
agreement. Prepared behavior remains authoritative for missing route facts,
invalid route references, duplicate or conflicting route facts, ambiguous
facts, route/prepared mismatch, policy-sensitive paths, diagnostics, wrappers,
printer/debug rows, helper-oracle behavior, and expected strings.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Keep the selected route surface narrow; if the implementation needs more
  than one route family or more than one selected consumer/helper, stop for
  plan review.
- Prefer route/prepared agreement gates over replacing prepared fallback.
- Add or adjust tests only for the selected semantic identity replacement and
  retained fallback/output behavior.
- Treat testcase-shaped matching, broad route facade work, expectation
  rewrites, unsupported downgrades, helper renames, or baseline refreshes as
  route-quality failures.
- For code-changing steps, run build proof plus the supervisor-selected narrow
  test subset. Escalate to broader backend validation if the diff touches
  shared route, lookup, printer/debug, wrapper, target-policy, or backend
  behavior beyond the selected helper.

## Ordered Steps

### Step 1: Select One Route Identity Surface

Goal: choose exactly one route family and one consumer/helper that can use
route/prepared agreement without absorbing prepared policy.

Primary targets:

- source and tests for the candidate route identity helper surfaces
- route-specific closure notes for the selected candidate only

Actions:

- Inspect the candidate helper surfaces listed in this runbook.
- Identify current callers, route facts, prepared fallback paths, and tests
  for plausible candidates.
- Choose one route family and one selected consumer or identity helper.
- Record in `todo.md` why the selected surface is identity-only and which
  prepared policy/fallback/output surfaces remain out of scope.
- If no candidate can be isolated without broad route migration or policy
  movement, stop and report the blocker.

Completion check:

- `todo.md` names exactly one route family and one selected consumer/helper.
- The selected surface has an explicit route/prepared agreement owner and a
  retained prepared fallback/policy owner.
- No implementation files have been changed by this step unless the supervisor
  explicitly delegates a combined inspect-and-edit packet.

### Step 2: Add Agreement-Gated Route Identity Read

Goal: route the selected consumer/helper through the selected route identity
fact when it agrees with prepared authority, while preserving prepared
behavior for all non-agreement paths.

Primary targets:

- selected helper or reader implementation
- narrow positive and fallback tests for the selected route family

Actions:

- Implement the smallest route/prepared agreement-gated identity read for the
  selected surface.
- Preserve prepared fallback for absent, invalid, ambiguous/conflict,
  mismatch, and policy-sensitive cases.
- Keep target policy, prepared mechanics, output, wrappers, printer/debug rows,
  helper-oracle behavior, and expected strings byte-stable.
- Avoid generic route facades, aggregate replacement, API retirement, helper
  renames, and expectation rewrites.

Completion check:

- The selected consumer/helper has one duplicate semantic route identity read
  replaced or contracted.
- Prepared fallback remains authoritative for every non-agreement path.
- Build proof and the supervisor-selected narrow test subset pass.

### Step 3: Prove Required Fallback, Output, And Nearby Coverage

Goal: prove the selected change is semantic, preserves policy/output behavior,
and is not shaped around one named testcase.

Primary targets:

- tests for the selected route identity surface
- printer/debug, wrapper, helper-oracle, or expected-string tests only when
  the selected helper reaches those surfaces

Actions:

- Add or confirm positive route/prepared agreement proof.
- Add or confirm absent, invalid, ambiguous/conflict, and mismatch proof.
- Add or confirm policy-sensitive fallback proof for the selected route family.
- Add or confirm output proof for printer/debug rows, wrapper output,
  helper-oracle behavior, and expected strings when those surfaces are
  reachable.
- Add or confirm at least one nearby same-feature route identity case.
- Do not rewrite expectations to mask behavior changes.

Completion check:

- The proof set covers positive agreement, absent, invalid,
  ambiguous/conflict, mismatch, policy-sensitive fallback, output stability,
  and nearby same-feature behavior.
- The proof does not weaken supported-path status, baselines, diagnostics,
  helper-oracle names, wrappers, printer/debug rows, or expected strings.

### Step 4: Validate And Hand Off For Lifecycle Review

Goal: make the selected route identity helper slice acceptance-ready and
decide whether idea 220 is complete or needs another lifecycle action.

Primary targets:

- code and tests touched by Steps 2 and 3
- `todo.md`

Actions:

- Run fresh build proof.
- Run the supervisor-selected narrow proof command.
- Escalate to a broader backend validation subset if the diff touches shared
  route, lookup, printer/debug, wrapper, target-policy, or backend behavior
  beyond the selected helper.
- Record final proof, residual risks, and any remaining route identity helper
  candidates in `todo.md`.
- Ask plan-owner lifecycle review to decide whether idea 220 is complete,
  should remain active for another helper, or should be deactivated/replaced.

Completion check:

- `todo.md` records final validation commands and results.
- The implementation is ready for supervisor review.
- Any remaining route identity helper work is explicitly listed as residual
  source-idea scope, not silently absorbed into this selected slice.
