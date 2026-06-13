# Phase E4 Cross-Target Wrapper Convergence Readiness Runbook

Status: Active
Source Idea: ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md

## Purpose

Prepare an analysis-only E4 readiness payload that decides which exact x86 or
riscv wrapper/debug/handoff boundary, if any, is ready for a separate
one-surface convergence implementation idea.

## Goal

Produce `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
and any accepted follow-up ideas under `ideas/open/`, without directly
migrating wrappers or claiming broad cross-target convergence.

## Core Rule

E4 may consume already-proven BIR, AArch64, or route-native semantic facts, but
it must preserve target-local ABI, frame, register, formatting,
instruction-selection, emission policy, prepared fallback, diagnostics/oracles,
expected strings, wrapper output, and baseline authority.

## Read First

Read the source idea and these required inputs before drafting conclusions:

- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md`
- `ideas/closed/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- E1/E2 closed implementation helpers `219`, `220`, `224`, and `225`
- E3 closed follow-ups `227` through `236`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- current accepted baseline state and hook review state

## Current Targets

- x86 Route 6 scalar `i32` argument-source route-debug row and
  `ConsumedPlans` compatibility.
- x86 compare-join stack-home handoff surface repaired by idea `234`.
- Route 6 consumed scalar `i32` call-argument source facts repaired by idea
  `235`.
- prepared selected-value-chain metadata repaired by idea `236`.
- x86 joined-branch and handoff wrapper rows adjacent to Route 5 edge/join and
  Route 7 comparison evidence.
- riscv prepared edge-publication emission and wrapper output surfaces that
  only have no-change proof so far.
- any wrapper-adjacent row named by E0/E1/E2/E3 as a cross-target prerequisite.

## Non-Goals

- Do not implement convergence in this activation.
- Do not perform route-wide x86 or riscv migration.
- Do not invent target-only BIR facts that bypass shared semantic ownership.
- Do not move target ABI, frame layout, register allocation, formatting,
  instruction selection, emission policy, branch spelling, call wrapper policy,
  or wrapper output into target-neutral BIR.
- Do not contract broad wrapper families.
- Do not perform prepared diagnostic/oracle replacement, E2 public/private API
  contraction, aggregate `PreparedFunctionLookups`, `PreparedBirModule`, draft
  155, or E5 retirement work.
- Do not use baseline refreshes, expected-string rewrites, helper renames,
  unsupported downgrades, timeout masking, or wrapper-output relabeling as
  proof.

## Working Model

Classify each candidate boundary as exactly one of:

- ready to draft one wrapper convergence implementation idea
- needs another named AArch64 or route semantic proof first
- needs E3 diagnostic/oracle parity first
- retained target-local policy
- retained prepared fallback or compatibility adapter
- blocked by expected-string, wrapper-output, or baseline authority
- blocked for riscv because no matching AArch64-proven semantic boundary exists
- no-action

An accepted implementation follow-up must name exactly one target wrapper input,
route-debug row, handoff boundary, or wrapper-adjacent compatibility surface;
the already-proven route/BIR fact it consumes; retained target-local policy and
prepared fallback; and the proof matrix required before wrapper ownership can
change.

## Execution Rules

- Keep this activation analysis-only except for writing the E4 document and
  any one-boundary follow-up idea files.
- Prefer direct citations to the required inputs when recording readiness or
  deferral reasons.
- Treat x86 repair evidence as x86-local unless a shared semantic boundary is
  proven.
- Treat riscv as blocked unless the required AArch64 or route semantic boundary
  exists and target-local emission/output policy remains retained.
- Preserve byte-stable wrapper output, route-debug output, helper-oracle
  behavior, expected strings, supported-path status, and baseline behavior.
- Escalate any tempting expectation downgrade, helper rename, or named-case-only
  shortcut as a reject signal, not as progress.
- When writing follow-up ideas, include concrete reviewer reject signals tied to
  the single boundary being proposed.

## Ordered Steps

### Step 1: Build the E4 Evidence Map

Goal: collect the prior-phase evidence needed to judge wrapper/debug/handoff
candidate readiness.

Primary targets:

- all files listed in `Read First`
- baseline and hook-review state visible in the current checkout

Actions:

- inspect each required input for cross-target wrapper, route-debug,
  diagnostic/oracle, prepared fallback, expected-string, and baseline authority
  constraints
- record the proven semantic facts available from E0 through E3
- separate AArch64-proven or route-native facts from x86 repair-only evidence
- note missing files or ambiguous authority as blockers in `todo.md`, not in the
  source idea

Completion check:

- the executor can name the evidence available for each candidate family and
  the authority surfaces that must remain target-local.

### Step 2: Draft the E4 Readiness Document Skeleton

Goal: create or refresh
`docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
with the required analysis sections.

Primary target:

- `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`

Actions:

- write a concise framing section linking this document to idea 237
- add a candidate-by-candidate readiness table
- add retained-authority sections for target-local policy, prepared fallback,
  diagnostics/oracles, expected strings, and baseline surfaces
- add deferral sections for E1, E2, E3, Route 8, E5, draft 155, and broad
  wrapper migration where relevant
- leave implementation follow-up details provisional until Step 4

Completion check:

- the document exists and contains the required table and retained-authority
  structure without asserting final readiness before candidates are classified.

### Step 3: Classify Each Candidate Boundary

Goal: assign every E4 candidate one allowed readiness classification with
evidence and reject-signal reasoning.

Primary target:

- `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`

Actions:

- classify each current target from the source idea
- include the consumed proven fact, retained target-local authority, prepared
  fallback status, diagnostic/oracle status, expected-string status, and
  baseline authority for each row
- explicitly distinguish x86 readiness from riscv readiness
- mark broad, route-wide, aggregate, draft 155, or E5 conclusions as out of
  scope or deferred

Completion check:

- every candidate has exactly one allowed classification and a short reason
  tied to prior evidence or an explicit blocker.

### Step 4: Create Accepted One-Boundary Follow-Up Ideas

Goal: write durable implementation ideas only for boundaries proven ready by
the E4 classification.

Primary targets:

- `ideas/open/*.md`

Actions:

- for each accepted boundary, create one follow-up idea file
- scope each follow-up to exactly one wrapper input, route-debug row, handoff
  boundary, or wrapper-adjacent compatibility surface
- name the consumed route/BIR fact, retained target-local policy, retained
  prepared fallback, and required proof matrix
- include reviewer reject signals for named-case shortcuts, unsupported
  downgrades, expectation rewrites, broad wrapper migration, and retained old
  failure modes behind new names
- do not create follow-up ideas for blocked, no-action, retained-policy, or
  wrong-phase candidates

Completion check:

- each accepted readiness row either has exactly one matching follow-up idea or
  the document explains why no follow-up was opened.

### Step 5: Finalize Closure Notes and Validation

Goal: make the analysis payload reviewable and ready for supervisor lifecycle
closure assessment.

Primary targets:

- `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
- `todo.md`

Actions:

- ensure the E4 document links back to the source idea
- ensure the closure-ready summary contains the E4 document path, candidate
  table, accepted follow-up ideas, retained-authority decisions, deferrals, and
  proof requirements
- explicitly state that draft 155 / E5 and broad cross-target wrapper migration
  remain unopened
- run documentation-appropriate validation such as checking links and staged
  diff scope; no compiler validation is required unless implementation files
  were changed by mistake

Completion check:

- `todo.md` records the final analysis proof, implementation files remain
  untouched, and the supervisor has enough evidence to decide close or further
  lifecycle action.

## Reviewer Reject Signals

- Treating one x86 Route 6 proof as route-wide x86 readiness.
- Treating x86 repair work as riscv readiness.
- Creating riscv-only BIR adapters without shared AArch64 or route semantic
  ownership.
- Weakening wrapper tests, expected strings, supported-path status, helper
  names, or baselines to make convergence appear green.
- Moving target ABI/layout/register/formatting/emission policy into BIR or
  route authority.
- Opening draft 155 or claiming broad prepared retirement from E4 readiness.
