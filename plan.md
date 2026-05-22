# AArch64 Pointer-Derived Load Address Scaling Closure Runbook

Status: Active
Source Idea: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md

## Purpose

Refresh and close, if the lifecycle gate permits, the parked pointer-derived
load/address-scaling timeout idea.

## Goal

Confirm the repaired AArch64 pointer-derived load/address-scaling path still
has no live first bad fact, then either close the source idea through the
lifecycle gate or leave a precise parked handoff.

## Core Rule

Do not reopen pointer-derived load/address-scaling implementation work unless
fresh generated-code or focused backend evidence shows the old timeout-causing
address-scaling failure, or an equivalent pointer-derived load address
calculation failure, is live again.

## Read First

- `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`
- `test_before.log` and `test_after.log` if present
- `scripts/c4c_regression_guard.py` or the repo-local regression guard entry
  used by `c4c-regression-guard`
- Focused memory and AArch64 representative tests named in the source idea
  lifecycle notes

## Current Scope

- Refresh the focused pointer-derived load/address-scaling proof on the
  current tree.
- Compare any observed first bad fact with the source idea scope.
- Run or prepare the close-time regression gate if the source idea remains
  satisfied.
- Update lifecycle state according to the gate result.

## Non-Goals

- Do not edit implementation files unless Step 1 finds a fresh in-scope first
  bad fact.
- Do not change expectations, unsupported classifications, CTest
  registration, runner behavior, timeout policy, or proof-log policy.
- Do not reopen materialized pointer StoreLocal writeback, direct `LoadGlobal`
  select-store handling, recursive formal post-call repairs, semantic string
  loads, frontend admission, ABI composite work, or variadic/floating
  residuals without a separate source idea.
- Do not special-case `00181`, `Hanoi`, one tower, one stack offset, one
  register, or one emitted instruction neighborhood.

## Working Model

The source idea repaired the pointer-derived load/address-scaling timeout by
preserving the live index/result carrier when materializing immediate scales.
The latest lifecycle notes say the current tree passed focused memory operand
contracts, materialized pointer writeback guardrails, and AArch64 `00170`,
`00181`, and `00189`, with no in-scope first bad fact remaining. Closure is
deferred because an acceptance-grade matching regression guard has not accepted
archival closure.

## Execution Rules

- Treat `todo.md` as the packet scratchpad and keep this runbook stable unless
  the route itself changes.
- Preserve canonical log names when producing regression evidence:
  `test_before.log` and `test_after.log`.
- If a fresh failure is outside this source idea, do not absorb it here;
  record the classification for supervisor handoff.
- If closure is rejected only by regression-guard basis or monotonicity, park
  rather than inventing implementation work.

## Step 1: Refresh Pointer-Derived Load/Scaling Proof

Goal: verify whether the pointer-derived load/address-scaling timeout owner is
still absent.

Primary target: focused memory operand contracts, materialized pointer
writeback guardrails, and AArch64 `00170`, `00181`, and `00189`.

Actions:

- Build the current tree.
- Run the focused subset selected by the supervisor for this closure-refresh
  packet, using the latest source lifecycle note as the default target family:
  memory operand contracts, materialized pointer writeback coverage, and
  `c_testsuite_aarch64_backend_src_00170_c`,
  `c_testsuite_aarch64_backend_src_00181_c`, and
  `c_testsuite_aarch64_backend_src_00189_c`.
- If `00181` or a focused memory guard fails, inspect semantic BIR, prepared
  BIR, and generated AArch64 only enough to decide whether the first bad fact
  is in this idea's pointer-derived load/address-scaling scope.
- Record the proof command, pass/fail counts, and classification result in
  `todo.md`.

Completion check:

- `todo.md` states either that no in-scope first bad fact remains, or
  identifies the concrete pointer-derived load/address-scaling boundary
  requiring Step 2 implementation.

## Step 2: Repair Only If A Fresh In-Scope Failure Exists

Goal: repair a current, localized pointer-derived load/address-scaling failure
if Step 1 finds one.

Primary target: the localized AArch64 pointer-derived load address, scale or
index materialization, emitted load address, or timeout-causing consumer path.

Actions:

- Make the smallest semantic repair to the localized lowering rule.
- Add or update focused backend coverage for the failing pointer-derived
  load/address-scaling shape.
- Keep idea 360 starting-state output and idea 361 materialized pointer
  store-writeback evidence stable.
- Re-run the Step 1 focused proof and any supervisor-selected adjacent
  guardrails.

Completion check:

- Focused coverage proves the repaired address-scaling behavior, `00181`
  advances or passes, and `00170`, `00189`, the starting-state output, and
  materialized pointer store writeback remain stable.

## Step 3: Run Close Gate Or Park With Classification

Goal: decide whether the source idea can close from the current evidence.

Primary target: lifecycle state for
`ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`.

Actions:

- If Step 1 is green and no in-scope first bad fact remains, run the required
  close-time regression guard using matching before/after logs.
- If the guard accepts closure, move the source idea through the lifecycle
  close flow.
- If the guard rejects closure without a new in-scope failure, deactivate the
  runbook and leave a compact parked note in the source idea only if needed.
- If Step 1 exposes an out-of-scope first bad fact, stop and report the
  lifecycle handoff target instead of expanding this idea.

Completion check:

- Exactly one of these is true: the idea is closed, the runbook is deactivated
  with source intent preserved, or a blocker is reported with the exact
  lifecycle ambiguity.
