# AArch64 C-Testsuite 00205 Timeout Residual Runbook

Status: Active
Source Idea: ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md
Activated From: parked close-rejected state after idea 303 closure

## Purpose

Resolve the parked `00205` timeout owner without absorbing the later
value-materialization residual that was split out and closed separately.

## Goal

Confirm that the generated-code timeout mechanism is repaired, then close or
hand off this idea based only on the timeout/control-flow owner.

## Core Rule

This owner is only about the focused `00205` timeout caused by generated
AArch64 control-flow, compare, branch, or value-flow behavior. Do not claim
value-materialization, output-correctness, scalar operand-form, runner,
timeout-policy, or expectation progress under this idea.

## Read First

- Source idea:
  `ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md`
- Split source:
  `ideas/closed/303_aarch64_sign_extension_assembler_legality.md`
- Follow-up value-materialization owner:
  `ideas/closed/305_aarch64_ctestsuite_00205_value_materialization_residual.md`
- Parked source note: commit `2d8bbf8c8` repaired the timeout by fixing
  AArch64 fused sign-extension compare-branch lowering for loop bounds.
- Accepted post-timeout state: `00205` completes quickly, preserves legal
  `sxtw x9, w13`, emits conditional loop-header compares, and any later wrong
  output belongs to the split value-materialization owner.

## Current Targets

- AArch64 generated loop-bound compare and branch lowering for focused `00205`.
- Evidence that `00205` no longer returns to the 5-second timeout path.
- Existing canonical close-gate logs when they match the selected broader
  scope.

## Non-Goals

- Do not reopen idea 303 sign-extension legality unless illegal `sxtw` with a
  W destination returns.
- Do not absorb idea 305 value-materialization or output-correctness behavior.
- Do not reopen idea 302 scalar arithmetic/reduction machine-node operand-form
  repairs.
- Do not change implementation files, expectations, allowlists, unsupported
  classifications, timeout policy, runner behavior, proof-log policy, or CTest
  registration as lifecycle progress.
- Do not special-case filename `00205`, source line numbers, temporary labels,
  exact register names, or exact stack offsets.

## Working Model

The source idea is a parked close candidate. Its lifecycle note says the
timeout-specific repair already landed and that close was previously blocked
only by missing matching close-gate artifacts. Idea 305 is now closed, so the
remaining lifecycle work is to verify that idea 304's timeout acceptance
criteria and the close gate are satisfied.

## Execution Rules

- Treat `todo.md` as the live close-verification scratchpad.
- Use existing canonical `test_before.log` and `test_after.log` if they cover
  the same broader backend scope.
- If the close gate is ambiguous or mismatched, stop and report the exact log
  ambiguity instead of editing logs.
- Keep any wrong-output or value-materialization evidence under closed idea
  305; do not reopen this idea from later `00205` behavior unless the timeout
  mechanism returns.

## Steps

### Step 1: Verify Timeout Closure Evidence

Goal: decide whether idea 304 satisfies its source acceptance criteria.

Primary target: source lifecycle note, committed proof context, generated
assembly evidence, and canonical close-gate logs

Actions:

- Confirm the source idea records the timeout repair from commit `2d8bbf8c8`.
- Confirm focused proof evidence says `00205` completes quickly and does not
  return to the 5-second timeout path.
- Confirm generated loop-header compares remain conditional and legal
  sign-extension spelling remains preserved.
- Confirm the later wrong-output residual was split to and closed under idea
  305 instead of being counted as timeout work.
- Run the close-gate regression checker on matching canonical logs using the
  supervisor-selected close scope.
- Record whether the close gate is accepted or blocked.

Completion check:

- `todo.md` records whether idea 304 is a valid close candidate or names the
  exact remaining close ambiguity.

### Step 2: Close Or Report Blocker

Goal: remove active lifecycle state if the timeout owner is complete.

Primary target: `ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md`

Actions:

- If Step 1 verifies source completion and the regression guard passes, close
  the idea by moving it to `ideas/closed/` and deleting `plan.md` and
  `todo.md`.
- If either condition fails, keep the active lifecycle state and record the
  blocker in `todo.md`.

Completion check:

- The lifecycle state either has no active `plan.md` / `todo.md` with idea 304
  archived under `ideas/closed/`, or `todo.md` explains why close was rejected.
