# AArch64 Sign-Extension Assembler Legality Runbook

Status: Active
Source Idea: ideas/open/303_aarch64_sign_extension_assembler_legality.md
Activated From: parked close-rejected state after idea 305 closure

## Purpose

Resolve the parked AArch64 sign-extension assembler-legality owner without
absorbing later timeout or value-materialization residuals.

## Goal

Confirm that the illegal `sxtw` spelling with a 32-bit destination register is
repaired, then close or hand off this idea based only on sign-extension
legality evidence.

## Core Rule

This owner is only about AArch64 sign-extension width legality. Do not claim
scalar operand-form, timeout, value-materialization, runtime-output, runner,
or expectation progress under this idea.

## Read First

- Source idea:
  `ideas/open/303_aarch64_sign_extension_assembler_legality.md`
- Split source:
  `ideas/open/302_aarch64_scalar_machine_node_operand_forms.md`
- Follow-up timeout owner:
  `ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md`
- Follow-up value-materialization owner:
  `ideas/closed/305_aarch64_ctestsuite_00205_value_materialization_residual.md`
- Parked source note: commit `9e4565365` repaired the observed illegal
  `sxtw w9, w13` spelling to legal `sxtw x9, w13`; close was previously
  rejected only because matching close-gate logs were unavailable.

## Current Targets

- AArch64 sign-extension instruction selection and assembly spelling.
- Focused evidence around `c_testsuite_aarch64_backend_src_00205_c` only as the
  case that exposed the illegal spelling.
- Existing canonical close-gate logs when they match the selected broader
  scope.

## Non-Goals

- Do not reopen idea 302 scalar arithmetic/reduction machine-node operand-form
  repairs.
- Do not absorb idea 304 timeout behavior or idea 305 value-materialization
  behavior.
- Do not change implementation files, expectations, allowlists, unsupported
  classifications, timeout policy, runner behavior, proof-log policy, or CTest
  registration as lifecycle progress.
- Do not special-case filename `00205`, line numbers, temporary registers, or
  exact diagnostic text.

## Working Model

The source idea is a parked close candidate. Its lifecycle note says the
sign-extension legality repair already landed, and the only previous close
blocker was absent matching `test_after.log`. Since idea 305 is now closed and
matching backend close-gate logs exist, this runbook should first verify that
the source acceptance criteria and close gate are still satisfied, then request
closure.

## Execution Rules

- Treat `todo.md` as the live close-verification scratchpad.
- Use existing canonical `test_before.log` and `test_after.log` if they cover
  the same broader backend scope.
- If the close gate is ambiguous or mismatched, stop and report the exact log
  ambiguity instead of editing logs.
- Keep any residual after sign-extension legality under its split owner; do
  not reopen this idea from later `00205` behavior unless illegal sign
  extension spelling returns.

## Steps

### Step 1: Verify Sign-Extension Closure Evidence

Goal: decide whether idea 303 satisfies its source acceptance criteria.

Primary target: source lifecycle note, committed proof context, generated
assembly evidence, and canonical close-gate logs

Actions:

- Confirm the source idea records the repair from illegal `sxtw w9, w13` to
  legal `sxtw x9, w13`.
- Confirm later residuals were split to idea 304 and idea 305 rather than
  being counted as sign-extension work.
- Run the close-gate regression checker on matching canonical logs using the
  supervisor-selected close scope.
- Record whether the close gate is accepted or blocked.

Completion check:

- `todo.md` records whether idea 303 is a valid close candidate or names the
  exact remaining close ambiguity.

### Step 2: Close Or Report Blocker

Goal: remove active lifecycle state if sign-extension legality is complete.

Primary target: `ideas/open/303_aarch64_sign_extension_assembler_legality.md`

Actions:

- If Step 1 verifies source completion and the regression guard passes, close
  the idea by moving it to `ideas/closed/` and deleting `plan.md` and
  `todo.md`.
- If either condition fails, keep the active lifecycle state and record the
  blocker in `todo.md`.

Completion check:

- The lifecycle state either has no active `plan.md` / `todo.md` with idea 303
  archived under `ideas/closed/`, or `todo.md` explains why close was rejected.
