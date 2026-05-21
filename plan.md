# AArch64 Signed Remainder Lowering Runbook

Status: Active
Source Idea: ideas/open/365_aarch64_signed_remainder_lowering.md
Activated From: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md

## Purpose

Repair the `00143` runtime residual exposed after duplicate synthetic
select-label generation was fixed: signed `count % 8` lowering computes the
wrong selector.

## Goal

Make AArch64 signed remainder lowering compute `a - (a / b) * b` using the
original divisor for the multiply-subtract operand.

## Core Rule

Repair signed remainder lowering generally. Do not special-case `00143`, one
Duff's-device dispatch, literal `8`, one register, or one emitted
`sdiv`/`msub` sequence.

## Read First

- `ideas/open/365_aarch64_signed_remainder_lowering.md`
- `todo.md`
- `test_after.log`
- `build/c_testsuite_aarch64_backend/src/00143.c.s`
- AArch64 scalar div/rem lowering and scalar ALU materialization code
- focused backend tests for signed division/remainder or scalar ALU records

## Current Scope

- signed integer `%` lowering in AArch64
- dividend/divisor/quotient carrier preservation around `sdiv`
- `msub` operand selection for signed remainder
- focused coverage for signed remainder feeding switch or scalar consumers
- representative proof for `c_testsuite_aarch64_backend_src_00143_c`

## Non-Goals

- Do not continue synthetic select-label work under this plan.
- Do not reopen unsigned div/rem idea 350 unless fresh evidence proves the
  signed boundary also owns an unsigned residual.
- Do not repair later `00143` runtime failures after signed remainder advances
  without lifecycle classification.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.

## Working Model

Generated AArch64 currently computes the quotient for `39 / 8` as `4`, then
forms the remainder with `msub w13, w9, w9, w13`, effectively subtracting
`4 * 4` from the dividend. The remainder path must preserve or rematerialize
the original divisor and use it in the multiply-subtract, yielding
`39 - 4 * 8 == 7`.

## Execution Rules

- Start from the bad `sdiv`/`msub` sequence in generated `00143.c.s`.
- Trace the dividend, divisor, quotient, and remainder destination through
  selected records and emitted instructions.
- Add focused coverage before or with the repair for signed remainder
  materialization.
- Preserve unsigned div/rem behavior, scalar ALU materialization, switch
  lowering, and the idea 364 label uniqueness repair.
- If `00143` advances after signed remainder is fixed, record the new first
  bad fact in `todo.md` instead of expanding this idea silently.

## Steps

### Step 1: Localize Signed Remainder Operand Flow

Goal: identify the exact AArch64 lowering boundary that replaces the original
divisor with the quotient in the remainder `msub`.

Primary target: generated `00143.c.s`, selected AArch64 records for
`count % 8`, and scalar div/rem lowering helpers.

Actions:

- Locate the `count % 8` lowered sequence and record the expected dividend,
  divisor, quotient, and destination registers or homes.
- Trace whether the divisor is clobbered, not preserved, or incorrectly mapped
  to the quotient register before `msub`.
- Name the first backend boundary where the wrong operand fact becomes
  authoritative.
- Decide the focused coverage shape needed before repair.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and coverage
  requirement for signed remainder lowering.

### Step 2: Add Focused Signed Remainder Coverage

Goal: guard signed remainder materialization independently of `00143`.

Primary target: backend scalar ALU or instruction-dispatch coverage that can
assert the signed remainder `sdiv`/`msub` operand relationship.

Actions:

- Add or extend focused coverage for signed `%` with a nontrivial quotient and
  divisor.
- Assert the multiply-subtract uses the original divisor operand, not the
  quotient twice.
- Include a consumer shape such as switch selection or scalar publication if
  needed to expose the localized boundary.

Completion check:

- Focused coverage fails before the repair or directly guards the bad operand
  relation.

### Step 3: Repair Signed Remainder Lowering

Goal: make signed remainder lowering preserve and use the divisor correctly.

Primary target: the scalar div/rem lowering or operand-publication helper
localized in Step 1.

Actions:

- Implement the smallest general repair for signed remainder operand
  selection.
- Preserve deterministic register use where practical.
- Avoid changes to unsigned div/rem unless the localized helper is shared and
  focused proof covers both paths.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated AArch64 for the covered shape uses
  the original divisor in the signed remainder `msub`.

### Step 4: Prove Representative And Classify Residual

Goal: prove `00143` advances past the bad `count % 8` selector and classify
any next first bad fact.

Primary target: focused backend coverage and
`c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated build and proof command.
- Inspect generated `00143.c.s` for the corrected signed remainder sequence.
- If `00143` still fails, classify the next first bad fact and decide whether
  it remains in this idea or needs lifecycle handoff.
- Ask the supervisor whether broader backend-regex or regression-guard proof
  is needed before closure.

Completion check:

- `00143` no longer fails from `count % 8` producing selector `23`; any
  remaining failure is explicitly classified in `todo.md`.
