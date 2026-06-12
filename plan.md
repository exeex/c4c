# Phase E1 Control-Flow Identity Helper Contraction Runbook

Status: Active
Source Idea: ideas/open/219_phase_e1_control_flow_identity_helper_contraction.md

## Purpose

Move one control-flow identity-only helper or selected reader away from
prepared control-flow lookup authority and onto direct BIR or route identity
authority, while retaining prepared ownership for policy, diagnostics,
fallback, wrappers, printer/debug rows, and expected strings.

## Goal

Replace one duplicate semantic identity read in the `PreparedControlFlow`
family with BIR/route identity authority and prove unchanged behavior for
fallback, policy-sensitive, diagnostic, wrapper, oracle, and string surfaces.

## Core Rule

Select exactly one identity-only helper or one selected reader before code
changes. Do not retire `PreparedControlFlow` as an aggregate, delete public
prepared APIs, move branch/output policy into BIR or route authority, or use
baseline/expectation rewrites as proof.

## Read First

- `ideas/open/219_phase_e1_control_flow_identity_helper_contraction.md`
- `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`
- `src/backend/prealloc/control_flow.hpp`
- the implementation and tests for the selected helper or reader only

## Current Targets

- one selected identity-only helper or selected reader from the
  `PreparedControlFlow` family
- candidate surfaces:
  - `resolve_prepared_block_label_id(...)`
  - `find_prepared_control_flow_function(...)`
  - `find_prepared_control_flow_block(...)`
  - `find_prepared_linear_join_predecessor(...)`
  - `find_prepared_control_flow_branch_target_labels(...)`
  - one selected branch/join facade helper

## Non-Goals

- Do not retire aggregate `PreparedControlFlow`.
- Do not delete public prepared APIs.
- Do not touch `PreparedBirModule` or `PreparedFunctionLookups` retirement.
- Do not change branch spelling, edge-copy scheduling, block-order emission,
  move-bundle policy, join transfer records, wrapper behavior, route-debug
  rows, prepared printer/debug rows, diagnostics, helper-oracle strings, or
  expected strings.
- Do not open Route 8 return-chain work unless a separate source idea imports
  a return schema.
- Do not perform liveness, intrinsic metadata, aggregate forwarding,
  diagnostic/oracle row replacement, E2, E3, E4, draft 155, or E5 work.
- Do not use baseline refreshes, unsupported downgrades, helper renames,
  timeout masking, or expectation rewrites as proof.

## Working Model

The selected slice must distinguish identity facts from retained prepared
policy. BIR/route may own only the chosen function/block identity, block label,
terminator successor, predecessor/successor, or selected edge/join identity
fact. Prepared behavior remains authoritative for absent evidence, invalid
references, duplicate/conflict evidence, BIR/route mismatch, policy-sensitive
paths, diagnostics, wrappers, printer/debug rows, helper-oracle behavior, and
expected strings.

## Execution Rules

- Keep routine progress and proof in `todo.md`.
- Keep the selected helper family narrow; if more than one helper or reader is
  required, stop and request plan review.
- Prefer agreement-gated reads over replacing prepared fallback.
- Add or adjust tests only to prove the semantic identity replacement and
  retained fallback behavior.
- Treat testcase-shaped matching, expected-string rewrites, or unsupported
  downgrades as route-quality failures.
- For code-changing steps, run build proof plus the supervisor-selected narrow
  test subset. Escalate to broader validation if the implementation touches
  shared control-flow, route, printer/debug, wrapper, or backend behavior
  beyond the chosen helper.

## Ordered Steps

### Step 1: Select One Control-Flow Identity Surface

Goal: choose the exact helper or reader that can move to BIR/route identity
authority without absorbing prepared policy.

Primary targets:

- `src/backend/prealloc/control_flow.hpp`
- implementation and tests for candidate control-flow helper surfaces

Actions:

- Inspect the candidate helper surfaces named in this runbook.
- Identify current callers and tests for each candidate.
- Choose one identity-only helper or selected reader for this runbook.
- Record in `todo.md` why the chosen helper is identity-only and which
  prepared policy/fallback surfaces remain out of scope.
- If no candidate can be isolated without policy migration, stop and report the
  blocker instead of broadening the plan.

Completion check:

- `todo.md` names exactly one selected helper or reader.
- The selected helper has an explicit BIR/route identity owner and retained
  prepared fallback/policy owner.
- No implementation files have been changed by this step unless the supervisor
  explicitly delegated a combined inspect-and-edit packet.

### Step 2: Add Agreement-Gated Identity Read

Goal: route the selected helper or reader through the BIR/route identity fact
when it agrees with prepared authority, while preserving prepared behavior for
all non-agreement paths.

Primary targets:

- selected helper or reader implementation
- narrow tests for positive agreement and retained fallback behavior

Actions:

- Implement the smallest agreement-gated BIR/route identity read for the
  selected surface.
- Preserve prepared behavior for absent, invalid, duplicate/conflict, and
  mismatch cases.
- Keep branch spelling, edge-copy scheduling, move policy, wrapper output,
  printer/debug rows, helper-oracle output, and expected strings byte-stable.
- Avoid helper renames, facade-only replacements, aggregate retirement, and
  public API deletion.

Completion check:

- The selected helper or reader has one duplicate semantic identity read
  replaced or contracted.
- Prepared fallback remains authoritative for every non-agreement path.
- Build proof and the supervisor-selected narrow test subset pass.

### Step 3: Prove Required Fallback And Nearby Coverage

Goal: prove the slice is semantic and general enough, not shaped around one
known testcase.

Primary targets:

- tests for the selected control-flow identity surface
- helper-oracle, printer/debug, wrapper, or expected-string tests only if the
  selected helper affects them

Actions:

- Add or confirm positive agreement proof.
- Add or confirm absent, invalid, duplicate/conflict, and mismatch proof.
- Add or confirm policy fallback proof for branch spelling, edge-copy
  scheduling, move policy, wrapper output, printer/debug rows, helper-oracle
  output, and expected strings when those surfaces are reachable.
- Add or confirm at least one nearby same-feature control-flow identity case.
- Do not rewrite expectations to mask behavior changes.

Completion check:

- The selected proof set covers positive, absent, invalid, duplicate/conflict,
  mismatch, policy fallback, and nearby same-feature behavior.
- The proof does not weaken supported-path status, baselines, diagnostics,
  helper-oracle names, or expected strings.

### Step 4: Validate And Hand Off For Lifecycle Review

Goal: make the selected implementation slice acceptance-ready and decide
whether this source idea is complete or needs another runbook.

Primary targets:

- code and tests touched by Steps 2 and 3
- `todo.md`

Actions:

- Run fresh build proof.
- Run the supervisor-selected narrow proof command.
- Escalate to a broader backend or control-flow validation subset if the diff
  touches shared control-flow, route, printer/debug, wrapper, or backend
  behavior beyond the selected helper.
- Record final proof, residual risks, and any remaining candidate helpers in
  `todo.md`.
- Ask plan-owner lifecycle review to decide whether idea 219 is complete,
  should remain active for another helper, or should be deactivated/replaced.

Completion check:

- `todo.md` records final validation commands and results.
- The implementation is ready for supervisor review.
- Any remaining control-flow identity helper work is explicitly listed as
  residual source-idea scope, not silently absorbed into this slice.
