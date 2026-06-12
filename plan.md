# Phase E3 Prepared Diagnostic/Oracle Replacement Readiness

Status: Active
Source Idea: ideas/open/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md

## Purpose

Decide which prepared diagnostic, printer/debug, route-debug,
helper-oracle, or expected-string row is ready for a later route/BIR-native
replacement or augmentation idea after E1 and E2 proved selected semantic
identity and private pass-context boundaries.

## Goal

Produce the Phase E3 readiness analysis and any accepted follow-up
implementation ideas, each scoped to exactly one diagnostic/oracle/string row
or one tightly scoped row family, without changing diagnostics, baselines,
prepared fallback behavior, or emitted output.

## Core Rule

E3 is analysis-only. It may classify diagnostic/oracle readiness and draft
follow-up ideas, but it must not directly replace prepared diagnostics,
refresh baselines, weaken oracle authority, rewrite expected strings, or move
target/emitted-output policy into route or BIR authority.

## Read First

- `ideas/open/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `ideas/closed/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md`
- `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`
- `ideas/closed/218_phase_e1_semantic_duplicate_candidate_triage.md`
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- `ideas/closed/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`
- `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md`
- `ideas/closed/220_phase_e1_route_identity_helper_contraction.md`
- `ideas/closed/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`
- `ideas/closed/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md`
- Route 3 through Route 7 row/diagnostic closures `208`, `210`, `212`,
  `214`, and `216`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`

## Current Targets

- Helper-oracle rows for
  `find_prepared_control_flow_branch_target_labels(...)`
- Helper-oracle, branch-control, and machine-printer rows around
  `find_prepared_fused_compare_operand_producer_facts(...)`
- Route 3 memory/source helper-oracle or prepared addressing printer rows
- Route 4 block-entry publication printer/debug rows
- Route 5 edge/join helper-oracle or prepared printer rows
- Route 6 x86 scalar source route-debug rows
- Route 7 fused-compare or materialized-condition helper-oracle rows
- E0/E1 diagnostic/oracle candidates that already have a proven semantic owner
  and fallback boundary
- Durable output document:
  `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`

## Non-Goals

- Do not implement any row replacement or augmentation.
- Do not broadly replace prepared printer, CLI dump, route-debug, or
  helper-oracle surfaces.
- Do not refresh baselines, rename helpers as proof, mask timeouts, downgrade
  supported paths, or rewrite expected strings.
- Do not contract public prepared APIs; that belongs to E2.
- Do not migrate cross-target wrappers; that belongs to E4.
- Do not open draft 155, E5 retirement, `PreparedFunctionLookups`,
  `PreparedBirModule`, or aggregate prepared-retirement work.
- Do not move target ABI/layout/register/stack/address/move/call/branch or
  emission policy into BIR or route authority.

## Working Model

- E0 keeps prepared diagnostics and string authority as the comparison surface
  for route/prepared agreement.
- E1/E2 prove only selected semantic identity and private pass-context
  boundaries.
- E3 identifies rows where route/BIR facts can explain a positive diagnostic
  while prepared fallback/oracle authority remains intact for non-agreement
  paths.
- A row can be ready for a follow-up idea only when the positive route/BIR
  evidence, retained fallback authority, and required proof matrix are all
  explicit.

## Execution Rules

- Treat this active runbook as analysis-only unless a later lifecycle action
  activates one scoped implementation idea.
- Prefer candidate-by-candidate classification over broad route-family claims.
- Preserve byte-stable printer/debug, route-debug, helper-oracle, wrapper, and
  expected-string behavior unless a later implementation idea separately
  proves a semantic output change.
- Classify each candidate as one of: ready to draft one implementation idea,
  proof harness only, retained prepared oracle/fallback authority, retained
  target/prepared policy or emitted-output authority, cross-target wrapper
  prerequisite owned by E4, blocked by expected-string/baseline/unsupported
  authority, or no-action.
- Accepted follow-up ideas must name one row or one tightly scoped row family,
  the positive BIR/route facts, retained prepared fallback/oracle authority,
  and the proof required before ownership can change.
- If execution writes new follow-up ideas under `ideas/open/`, include concrete
  reviewer reject signals against testcase-shaped shortcuts, expectation
  rewrites, unsupported downgrades, broad replacements, and target-policy
  migration.

## Steps

### Step 1: Assemble Evidence Inventory

Goal: collect the E0, E1, E2, Route 3 through Route 7, and retained-surface
evidence needed to classify diagnostic/oracle rows.

Primary targets:

- Required input idea and doc files listed in `Read First`
- Current accepted baseline state and hook review state

Actions:

- Read the required inputs and record in `todo.md` the proven semantic owner,
  private-boundary result, retained prepared authority, and relevant row
  families for each source.
- Identify which E1/E2 closures prove branch-target identity, fused-compare
  operand producer identity, or earlier Route 3 through Route 7 row evidence.
- Record baseline and hook-review state as analysis context only; do not
  refresh baselines or mutate hook state.

Completion check:

- `todo.md` contains an evidence inventory with source file references and the
  diagnostic/oracle row families each source can support.
- No production, test, baseline, expected-string, source-idea, or output-doc
  changes are made in this step.

### Step 2: Classify Candidate Rows

Goal: classify every candidate row or tightly scoped row family against the E3
readiness categories.

Primary targets:

- Candidate surfaces named in the source idea
- Any E0/E1 row-scoped diagnostic/oracle candidate with a proven semantic
  owner and fallback boundary

Actions:

- Build a candidate-by-candidate table in `todo.md`.
- For each candidate, name the positive BIR/route fact if one exists.
- For each candidate, name the prepared fallback/oracle, target-policy,
  wrapper, expected-string, baseline, unsupported-path, or emitted-output
  authority that must remain retained.
- Mark candidates that are only proof harnesses, E4 wrapper prerequisites, or
  no-action surfaces.

Completion check:

- `todo.md` has a complete candidate classification table.
- Each positive candidate names the exact row or tightly scoped row family and
  the reason it is not broader than E3 permits.
- Each rejected or deferred candidate names the blocking retained authority or
  owning future phase.

### Step 3: Draft The E3 Readiness Document

Goal: write the durable Phase E3 analysis document from the completed
inventory and classification.

Primary target:

- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`

Actions:

- Write the candidate-by-candidate diagnostic/oracle readiness table.
- Include accepted follow-up implementation candidates, if any, each scoped to
  one row or one tightly scoped row family.
- Include explicit retained-authority decisions for prepared fallback/oracle,
  target policy, wrappers, expected strings, and baseline surfaces.
- Include deferrals to E1, E2, E4, Route 8, or E5 where E3 is not the correct
  owner.
- State explicitly that draft 155 / E5 and broad diagnostic/oracle replacement
  remain unopened.

Completion check:

- The durable E3 analysis document exists and matches the source idea's
  expected-output contract.
- The document does not claim implementation progress, baseline changes,
  prepared retirement, or broad diagnostic/oracle replacement.

### Step 4: Create Accepted Follow-Up Ideas

Goal: create any implementation follow-up ideas accepted by the E3 analysis
without starting their implementation.

Primary targets:

- `ideas/open/*.md` follow-up files for accepted one-row or tightly scoped
  row-family replacement/augmentation ideas
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`

Actions:

- For each accepted follow-up, write a durable source idea that names the row
  or tightly scoped row family, the positive BIR/route facts, retained prepared
  fallback/oracle authority, and required proof matrix.
- Include reviewer reject signals tailored to that row family.
- Link the follow-up ideas from the E3 readiness document.
- Do not activate or implement any follow-up idea during E3 unless the
  supervisor performs a separate lifecycle transition.

Completion check:

- Every accepted follow-up from the E3 document has a matching open idea.
- No broad replacement, expected-string rewrite, baseline refresh,
  unsupported downgrade, or target-policy migration is included in any
  follow-up source idea.

### Step 5: Closeout Review

Goal: prepare the active E3 runbook for lifecycle closure after analysis and
follow-up idea creation are complete.

Actions:

- Re-read the source idea and verify the E3 output contract is satisfied.
- Verify `todo.md` links the readiness document, lists all accepted follow-up
  ideas, and records retained-authority decisions and deferrals.
- Check the diff for accidental implementation, baseline, expected-string,
  unsupported-path, or broad replacement changes.
- Hand the active plan to the plan owner for closure review.

Completion check:

- The readiness document and accepted follow-up ideas satisfy the source idea.
- `todo.md` contains final analysis status and proof/validation notes
  appropriate for an analysis-only lifecycle slice.
- The active runbook is ready for closure review, not implementation.
