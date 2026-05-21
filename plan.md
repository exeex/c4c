# AArch64 Short-Circuit Control Publication Regression Runbook

Status: Active
Source Idea: ideas/open/370_aarch64_short_circuit_control_publication_regression.md
Activated From: close-time full-suite baseline regression after idea 366

## Purpose

Repair the close-time full-suite regression where AArch64 now evaluates a
short-circuit right-hand operand in `00196`.

## Goal

Restore correct AArch64 short-circuit `&&`/`||` evaluation and keep the
recent `00112` string-pointer comparison fix intact.

## Core Rule

Repair the shared short-circuit/control publication boundary. Do not
special-case `00196`, `fred`, `joe`, one expression ordinal, one label, one
register, or one emitted text sequence.

## Read First

- `ideas/open/370_aarch64_short_circuit_control_publication_regression.md`
- `todo.md`
- `test_baseline.log`
- `test_after.log`
- `tests/c/external/c-testsuite/src/00196.c`
- generated `build/c_testsuite_aarch64_backend/src/00196.c.s`
- AArch64 short-circuit, compare publication, control-value publication, and
  recent pointer comparison materialization paths

## Current Scope

- new full-suite regression `c_testsuite_aarch64_backend_src_00196_c`
- short-circuit logical `&&`/`||` with function-call right-hand operands
- control-value and compare-result publication around branch decisions
- preservation check for `c_testsuite_aarch64_backend_src_00112_c`

## Non-Goals

- Do not broaden into unrelated backend inventory failures or old baseline
  failures.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.
- Do not weaken or revert the `00112` capability unless localization proves a
  precise regression cause and the replacement keeps `00112` passing.
- Do not treat unchanged aggregate pass/fail counts as success while `00196`
  is newly failing.

## Working Model

The accepted baseline did not fail `00196`; the current full-suite run does.
The failure is not a general source-level expectation issue because the native
`c_testsuite_src_00196_c` passes in the same run. The likely owner is AArch64
backend lowering around short-circuit branch/control publication, possibly
exposed by the recent generalization for pointer compare publication.

## Execution Rules

- Start from the focused `00196` mismatch and generated AArch64.
- Compare the expected short-circuit call order against emitted branch/control
  structure before changing code.
- Identify whether the right-hand operand is entered because the branch sense,
  published control value, compare input, or join label is wrong.
- Add focused backend coverage before or with the repair.
- Preserve `00112` and do not reintroduce stale string-pointer comparison
  returns.
- Keep proof commands explicit in `todo.md`; supervisor owns final baseline
  guard selection.

## Steps

### Step 1: Localize Short-Circuit Regression Boundary

Goal: identify the first AArch64 backend boundary where `fred() && (1 + joe())`
loses required short-circuit skip behavior.

Primary target: generated `00196.c.s`, prepared BIR/MIR for the fifth
expression, and AArch64 short-circuit/control publication helpers.

Actions:

- Inspect `test_after.log` for the `00196` mismatch and confirm the focused
  rerun failure shape.
- Inspect generated AArch64 for each `&&`/`||` expression, especially the
  fifth expression.
- Trace the branch condition, published control value, and join label for the
  left-hand `fred()` result.
- Identify whether recent pointer comparison publication changes are involved
  or merely adjacent.
- Decide the focused coverage shape needed before repair.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and coverage
  requirement for short-circuit function-call operand skipping.

### Step 2: Add Focused Short-Circuit Coverage

Goal: guard the regressed behavior independently of external `00196`.

Primary target: backend tests or c-testsuite-style focused coverage for
function-call short-circuit `&&`/`||` lowering.

Actions:

- Add coverage where a false left-hand `&&` operand must skip a right-hand
  function call wrapped in arithmetic.
- Add or extend coverage for the related `||` skip shape if the same owner
  boundary handles both.
- Assert behavior semantically through call order, return value, emitted
  branch/control structure, or the repo's established focused backend style.
- Keep the test independent of `fred`, `joe`, one printed output block, or one
  emitted register sequence.

Completion check:

- Focused coverage fails before the repair or directly guards the localized
  short-circuit/control publication fact.

### Step 3: Repair Short-Circuit Control Publication

Goal: restore general AArch64 short-circuit skip behavior while preserving the
string-pointer comparison repair.

Primary target: the AArch64 lowering helper localized in Step 1.

Actions:

- Implement the smallest semantic repair at the owning short-circuit/control
  publication boundary.
- Preserve branch sense, join labels, and result publication for both `&&` and
  `||` shapes covered by the owner.
- Avoid broad rewrites of unrelated pointer/address, aggregate, FP, file I/O,
  or timeout owners.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes, `00196` no longer evaluates skipped right-hand
  operands, and `00112` still passes.

### Step 4: Prove Baseline Regression Is Removed

Goal: prove the new full-suite regression is gone without trading away the
recent resolved test.

Primary target: `c_testsuite_aarch64_backend_src_00196_c`,
`c_testsuite_aarch64_backend_src_00112_c`, and the supervisor-selected
regression guard scope.

Actions:

- Run the supervisor-delegated build and proof command for `00196` and `00112`.
- Inspect generated `00196.c.s` enough to confirm the right-hand call is
  guarded by the left-hand result.
- If `00196` still fails, classify the next first bad fact in `todo.md`.
- Ask the supervisor whether full-suite or narrower matching guard proof is
  required before closure.

Completion check:

- `00196` passes, `00112` remains passing, and any broader regression-guard
  result is recorded in `todo.md`.
