# Phase E3 Branch-Target Helper-Oracle Follow-Up

Status: Active
Source Idea: ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md

## Purpose

Finish the selected `find_prepared_control_flow_branch_target_labels(...)`
helper-oracle branch-target row after Step 1 found that the agreement-reader
implementation path is already present, by validating that path and adding the
missing selected-row fallback proof.

## Goal

Prove the selected non-compare/materialized-bool AArch64 conditional branch
row uses BIR structured successor label identity only under prepared agreement,
and prove that all non-agreement paths retain prepared helper-oracle behavior.

## Core Rule

This plan may change one branch-target helper-oracle row family only. Do not
rewrite branch spelling, edge-copy scheduling, printer/debug output, wrappers,
expected strings, target policy, emitted-output policy, or broad prepared
diagnostic/oracle ownership.

## Read First

- `ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md`
- `ideas/closed/219_phase_e1_control_flow_identity_helper_contraction.md`
- `ideas/closed/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`
- `ideas/closed/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- Existing implementation and tests around:
  `find_prepared_control_flow_branch_target_labels(...)`,
  `BranchTargetIdentityPassContext`, and
  `read_agreeing_bir_branch_target_labels(...)`

## Current Targets

- One selected helper-oracle row family tied to
  `find_prepared_control_flow_branch_target_labels(...)`
- Positive BIR structured successor identity under prepared agreement
- Prepared fallback for absent context, invalid ids, duplicate or conflict
  cases, mismatch, non-conditional BIR, and non-agreement paths
- Nearby same-feature branch-target tests that prove this is not shaped around
  one named fixture

## Non-Goals

- Do not replace broad control-flow helper, printer/debug, wrapper,
  branch-control, edge-copy, or emitted-output behavior.
- Do not migrate branch spelling, branch scheduling, edge-copy scheduling,
  aggregate APIs, or target policy.
- Do not refresh baselines, rewrite expected strings, rename helpers as proof,
  downgrade supported paths, or weaken helper-oracle authority.
- Do not start E4 wrapper work, Route 8, draft 155, E5, or broad prepared
  diagnostic/oracle retirement.

## Working Model

- Phase E1 proved agreement-gated BIR structured successor label identity for
  the selected AArch64 conditional branch consumer.
- Phase E2 moved that read behind `BranchTargetIdentityPassContext` and
  `read_agreeing_bir_branch_target_labels(...)`.
- Phase E3 accepted this helper-oracle row family as ready for a scoped
  implementation idea, with prepared data retaining fallback and output
  authority.
- Step 1 found that the selected non-compare/materialized-bool row in
  `src/backend/mir/aarch64/codegen/comparison.cpp` already calls the private
  agreement reader around the prepared control-flow target lookup.
- The remaining work is proof-oriented: validate that the existing path is the
  intended selected row, then add missing selected-row fallback and
  nearby same-feature coverage.

## Execution Rules

- Treat Step 1's inventory in `todo.md` as the current discovery baseline.
- Do not add a no-op implementation slice for the already-present agreement
  reader path.
- Keep any code change behavior-preserving except for focused test scaffolding
  or narrowly required proof support for the selected row.
- Treat prepared output as authoritative whenever BIR evidence is absent,
  invalid, duplicate/conflicting, mismatched, non-conditional, or not in
  agreement.
- Prefer semantic use of `BranchTargetIdentityPassContext` and
  `read_agreeing_bir_branch_target_labels(...)`; do not add testcase-shaped
  matching or fixture-name shortcuts.
- Every implementation step needs fresh build or compile proof plus the
  delegated test subset. Escalate to broader validation before acceptance if
  the diff touches shared control-flow lowering, printer/debug, wrappers, or
  expected-output surfaces.

## Steps

### Step 1: Locate The Row Family And Proof Surface

Goal: identify the selected branch-target helper-oracle row family, its current
prepared-only behavior, and the tests that prove positive and fallback cases.

Primary targets:

- `find_prepared_control_flow_branch_target_labels(...)`
- `BranchTargetIdentityPassContext`
- `read_agreeing_bir_branch_target_labels(...)`
- Existing helper-oracle, branch-target, and nearby same-feature tests

Actions:

- Inspect the helper implementation and current callers to find the precise
  helper-oracle row family eligible for this plan.
- Map the positive prepared row to the available BIR structured successor
  identity evidence from the private pass context.
- Inventory current positive, absent-context, invalid-id, duplicate/conflict,
  mismatch, non-conditional, non-agreement, and public fallback tests.
- Record any proof gaps in `todo.md` before implementation begins.

Completion check:

- `todo.md` names the exact row family, implementation target files, and
  positive/fallback test coverage to use for Step 2.
- No code, baseline, expected-string, wrapper, printer/debug, or output-policy
  change is made in this step unless the executor is explicitly delegated to
  start implementation.

### Step 2: Validate Existing Agreement-Gated Evidence Path

Goal: confirm the already-present selected-row implementation is the intended
agreement-gated BIR evidence path and identify the smallest proof additions
still needed.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `find_prepared_conditional_branch_facts(...)`
- `BranchTargetIdentityPassContext`
- `read_agreeing_bir_branch_target_labels(...)`
- Step 1 inventory in `todo.md`

Actions:

- Inspect the selected non-compare/materialized-bool conditional branch row and
  confirm it consumes prepared branch targets, then replaces them only when
  `read_agreeing_bir_branch_target_labels(...)` returns agreeing labels.
- Confirm absent context, invalid ids, mismatched ids, conflicting structured
  ids, non-conditional BIR, and non-agreement still fail closed to prepared
  targets at the helper-reader boundary.
- Confirm the public two-argument prepared helper remains available and is not
  bypassed for prepared-only callers.
- Record the exact selected-row proof gaps that Step 3 must close; do not
  modify implementation unless validation exposes a real semantic defect.

Completion check:

- `todo.md` states whether the existing selected-row implementation satisfies
  the agreement-gated evidence contract.
- `todo.md` names the focused selected-row fallback or nearby same-feature
  tests to add in Step 3.
- No implementation, expected-string, baseline, wrapper, printer/debug, branch
  spelling, edge-copy, target-policy, or emitted-output change is made unless a
  real semantic defect is found and recorded.

### Step 3: Add Selected-Row Fallback And Nearby Same-Feature Proof

Goal: prove that the implementation is semantic and not shaped around one
named branch-target testcase.

Primary targets:

- Existing branch-target helper-oracle tests
- Nearby same-feature conditional branch-target cases
- Negative/fallback cases for missing, invalid, conflicting, mismatched, and
  non-conditional evidence

Actions:

- Add or adjust focused tests only where Step 2 confirms coverage is missing
  for the selected row family and fallback matrix.
- Prefer tests that assert unchanged prepared branch payloads while exercising
  the existing BIR agreement and fail-closed paths.
- Include at least one selected-row fallback case at the AArch64 consumer
  boundary, such as absent `context.bir_block`, invalid successor ids,
  mismatched ids, conflicting structured ids, non-conditional BIR, or raw BIR
  label text drift with agreeing structured ids.
- Include nearby same-feature proof or explicit existing-test mapping so the
  route is not accepted only through one named materialized-bool fixture.
- Do not weaken supported-path expectations, expected strings, wrappers,
  printer/debug output, branch-control output, or emitted output.

Completion check:

- Positive agreement and every required fallback path are covered by focused
  tests or explicit proof.
- Nearby same-feature coverage demonstrates the route is not a named-case
  shortcut.
- Expected strings, baselines, wrappers, printer/debug output, branch-control
  output, and emitted-output policy remain unchanged.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed helper-oracle row slice and leave clear handoff state
for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated narrow test subset
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact delegated proof command and record the command and result in
  `todo.md`.
- If the change touched shared control-flow, wrapper, printer/debug, expected
  string, or output-policy surfaces, ask the supervisor to choose broader
  validation before acceptance.
- Summarize the row family changed, fallback guarantees retained, and tests
  covering nearby same-feature behavior.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, retained
  authority, and any residual risks.
