# AArch64 Scalar Cast Stack-Homed Register Source Publication Refresh

Status: Active
Source Idea: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md

## Purpose

Refresh the parked closure-deferred state for idea 340 and decide whether
there is any current in-scope scalar-cast source-publication work left.

## Goal

Confirm whether AArch64 selected scalar casts still lose a structured register
source when the original source is stack-homed but a prepared consumer
stack-to-register move exists.

## Core Rule

Do not reopen adjacent scalar storage, frame layout, compare lowering, runtime
value correctness, expectation policy, runner behavior, CTest registration, or
proof-log policy unless fresh generated-code evidence shows the first bad fact
has moved there.

## Read First

- `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
- Focused scalar-cast backend tests for selected and prepared scalar cast
  records.
- The generated/prepared evidence for `c_testsuite_aarch64_backend_src_00143_c`
  if the representative fails again.

## Current Targets

- `backend_aarch64_scalar_cast_records`
- `backend_aarch64_prepared_scalar_cast_records`
- `c_testsuite_aarch64_backend_src_00143_c`

## Non-Goals

- Do not modify files under `ideas/closed/`.
- Do not repair the out-of-scope fallthrough fixed-offset local load/store
  residual unless a separate lifecycle transition owns that work.
- Do not special-case `00143`, `%t76`, `%t81`, value ids 308 or 388, `x13`,
  block 16, instruction 158, or one diagnostic string.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.

## Working Model

The original owner was selected scalar-cast source publication: a stack-homed
input had a prepared consumer stack-to-register move, but the selected
`sign_extend` cast reached AArch64 printing without a structured register
source operand. Prior lifecycle notes say this scope was repaired and the
remaining `00143` residual moved elsewhere.

## Execution Rules

- Treat Step 1 as a refresh/classification packet, not an implementation
  packet unless the old scalar-cast source-publication diagnostic is live.
- If all focused targets pass, update `todo.md` with the proof and route to
  Step 3 for close/deactivate decision.
- If `00143` fails, classify the first bad fact before editing code.
- If the first bad fact is out of scope, record the handoff owner in
  `todo.md` and route to Step 3.
- Any code-changing packet must include build proof plus the focused scalar
  cast subset before acceptance.

## Step 1: Refresh Current Scalar-Cast Proof

Goal: determine whether the old missing structured register-source diagnostic
or any in-scope scalar-cast source-publication failure is currently live.

Primary target:

- `c_testsuite_aarch64_backend_src_00143_c`

Actions:

- Run the focused proof for the current targets.
- If a target fails, inspect generated/prepared evidence enough to classify
  the first bad fact.
- Distinguish in-scope scalar-cast source-publication failures from
  fallthrough fixed-offset local load/store, frame layout, compare lowering,
  runtime value correctness, or unrelated residuals.

Completion check:

- `todo.md` records the focused command, pass/fail result, and either the
  current in-scope first bad fact or the absence of one.

## Step 2: Repair In-Scope Source Publication If Live

Goal: repair only a proven current scalar-cast source-publication defect.

Primary target:

- The selected scalar-cast publication, normalization, or selected-node
  handoff boundary identified by Step 1.

Actions:

- Repair the general source-publication rule so selected scalar casts consume
  the prepared register source when the original source is stack-homed.
- Add or update focused backend coverage for the repaired shape.
- Keep nearby scalar cast and prepared scalar cast contracts stable.

Completion check:

- Build passes.
- Focused scalar-cast backend coverage passes.
- `c_testsuite_aarch64_backend_src_00143_c` no longer fails with the old
  structured register-source diagnostic, or its new first bad fact is
  explicitly classified.

## Step 3: Lifecycle Close Or Deactivate Decision

Goal: decide whether idea 340 can close or must remain parked.

Actions:

- If the source idea is complete, run the required close-time regression guard
  using matching canonical logs.
- Accept closure only if the source idea is satisfied and the regression guard
  passes.
- If the focused proof is green but the strict close gate rejects closure for
  unchanged pass count, deactivate the runbook and leave the source idea open
  and parked.
- If a new out-of-scope first bad fact appears, record the handoff without
  expanding this source idea.

Completion check:

- The lifecycle state is closed, deactivated, or explicitly handed off without
  leaving ambiguous active execution state.
