# Phase E3 Branch-Target Helper-Oracle Follow-Up

Status: Active
Source Idea: ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md

## Purpose

Augment one selected `find_prepared_control_flow_branch_target_labels(...)`
helper-oracle branch-target label row family with BIR structured successor
identity evidence while preserving prepared fallback and output authority.

## Goal

Use BIR structured successor label identity only when it agrees with prepared
branch-target data, and prove that all non-agreement paths retain the current
prepared helper-oracle behavior.

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
- The implementation should consume the private agreement reader for the
  positive row and fail closed to the existing prepared result everywhere else.

## Execution Rules

- Start by identifying the exact helper-oracle row family and its existing
  tests before editing code.
- Keep each code change behavior-preserving except for the selected diagnostic
  evidence source under proven agreement.
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

### Step 2: Add Agreement-Gated BIR Evidence

Goal: route the selected positive helper-oracle row through BIR structured
successor identity only after prepared agreement.

Primary targets:

- The helper-oracle row family found in Step 1
- `BranchTargetIdentityPassContext`
- `read_agreeing_bir_branch_target_labels(...)`

Actions:

- Thread or consume the private branch-target identity context at the selected
  helper-oracle boundary using existing local patterns.
- Use `read_agreeing_bir_branch_target_labels(...)` for the positive
  agreement path.
- Preserve the existing prepared helper result for absent context, invalid ids,
  duplicates/conflicts, mismatches, non-conditional BIR, non-agreement, and
  prepared-only fallback.
- Keep public prepared helper fallback available and byte-stable.

Completion check:

- The selected positive row can be explained by BIR structured successor
  identity under prepared agreement.
- All non-agreement paths retain the same prepared helper-oracle authority and
  externally visible behavior.
- The slice builds and the delegated narrow proof passes without expectation or
  baseline rewrites.

### Step 3: Cover Fallback And Nearby Same-Feature Cases

Goal: prove that the implementation is semantic and not shaped around one
named branch-target testcase.

Primary targets:

- Existing branch-target helper-oracle tests
- Nearby same-feature conditional branch-target cases
- Negative/fallback cases for missing, invalid, conflicting, mismatched, and
  non-conditional evidence

Actions:

- Add or adjust focused tests only where coverage is missing for the selected
  row family and fallback matrix.
- Prefer tests that assert unchanged helper-oracle behavior while exercising
  the new BIR agreement path.
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
