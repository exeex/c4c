# AArch64 Scalar Machine Node Operand Forms Runbook

Status: Active
Source Idea: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md
Activated From: parked reactivation after ideas 303, 304, and 305 closure

## Purpose

Resolve the parked AArch64 scalar arithmetic/reduction machine-node
operand-form owner after its split follow-up owners have been closed.

## Goal

Determine whether the focused scalar `div`, scalar `mul`, and scalar
`logical_shift_right` unsigned-reduction operand-form diagnostics for `00064`,
`00139`, and `00205` are already repaired, then either close the idea or
delegate the remaining semantic operand-form repair.

## Core Rule

This owner is only about scalar arithmetic and reduction machine-node operand
publication, selection, materialization, or structured printer admission. Do
not claim progress from sign-extension legality, timeout repair,
value-materialization repair, runtime output behavior, expectations,
allowlists, unsupported classifications, timeout policy, runner behavior, or
CTest registration.

## Read First

- Source idea:
  `ideas/open/302_aarch64_scalar_machine_node_operand_forms.md`
- Umbrella split source:
  `ideas/open/295_backend_regex_failure_family_inventory.md`
- Split follow-up owners:
  - `ideas/closed/303_aarch64_sign_extension_assembler_legality.md`
  - `ideas/closed/304_aarch64_ctestsuite_00205_timeout_residual.md`
  - `ideas/closed/305_aarch64_ctestsuite_00205_value_materialization_residual.md`
- Source parked note: do not close this idea or claim focused pass-count
  progress from the dirty Step 3 implementation alone; reactivation must first
  review the committed Step 3 slice, its focused proof, and whether the old
  `00205` unsigned-reduction diagnostic is absent without absorbing the split
  sign-extension owner.

## Current Targets

- Focused cases: `00064`, `00139`, and `00205`.
- Old diagnostics:
  - scalar `div` operand forms in `00064`;
  - scalar `mul` operand forms in `00139`;
  - scalar `logical_shift_right` unsigned-reduction operand forms in `00205`.
- Current committed proof and generated diagnostics/assembly for the focused
  family.
- Matching broader backend close-gate logs if this owner becomes a close
  candidate.

## Non-Goals

- Do not reopen closed fused compare-branch owner 296, scalar immediate owner
  299, scalar-cast owner 300, memory-store owner 301, sign-extension owner 303,
  timeout owner 304, or value-materialization owner 305 without generated-code
  evidence contradicting their closure boundaries.
- Do not absorb assembly legality/materialization singletons `00104` and
  `00182`, call-boundary move gap `00140`, `lir_to_bir` residuals `00204` and
  `00216`, runtime residuals, timeout/output-storm cases, libc, floating, ABI,
  aggregate, pointer, string, or control-flow buckets.
- Do not edit implementation files, expectations, allowlists, unsupported
  classifications, timeout policy, runner behavior, proof-log policy, or CTest
  registration as lifecycle progress.
- Do not special-case `00064`, `00139`, `00205`, exact diagnostic strings, or
  temporary register names.

## Working Model

Idea 302 was parked after the scalar machine-node operand-form route moved
`00205` past the old logical-shift-right unsigned-reduction diagnostic and
exposed separate owners: sign-extension legality, timeout control-flow, and
value materialization. Those split owners are now closed. The next lifecycle
step is not to assume completion from their closure alone, but to verify the
original scalar operand-form acceptance criteria against committed proof for
the full focused family.

## Execution Rules

- Treat `todo.md` as the live verification scratchpad.
- Start by reviewing committed proof and current diagnostics for `00064`,
  `00139`, and `00205`; do not edit implementation during verification.
- If the old scalar operand-form diagnostics are absent and the focused family
  has fresh proof, request close-gate validation and closure.
- If any old scalar `div`, scalar `mul`, or scalar `logical_shift_right`
  operand-form diagnostic remains, delegate a narrow semantic repair under
  this idea.
- If the focused cases fail for a different residual, record it in `todo.md`
  and split or hand off instead of expanding this owner.

## Steps

### Step 1: Reactivation Review And Focused Evidence Check

Goal: decide whether idea 302 is a close candidate or still needs scalar
operand-form implementation.

Primary target: committed proof, current focused diagnostics, and generated
AArch64 output for `00064`, `00139`, and `00205`

Actions:

- Review the committed Step 3 scalar operand-form work referenced by the
  parked source note.
- Confirm whether focused proof now covers all three cases: `00064`, `00139`,
  and `00205`.
- Check whether the old scalar `div`, scalar `mul`, and scalar
  `logical_shift_right` unsigned-reduction machine-node operand-form
  diagnostics are absent.
- Confirm any later failures belong to closed split owners or separate parked
  buckets, not to this scalar operand-form owner.
- If current canonical logs are suitable, run the close-gate regression
  checker; otherwise record the exact proof/log gap.

Completion check:

- `todo.md` records whether idea 302 is a valid close candidate or names the
  exact remaining scalar operand-form diagnostic/proof blocker.

### Step 2: Repair Remaining Scalar Operand Forms Or Close

Goal: either execute the focused semantic repair or close the satisfied owner.

Primary target: AArch64 scalar arithmetic/reduction operand publication,
selection, materialization, or printer admission

Actions:

- If Step 1 finds a remaining scalar operand-form diagnostic, repair the
  backend path semantically across the focused family without testcase-shaped
  matching.
- If Step 1 verifies source completion and the regression guard passes, close
  the idea by moving it to `ideas/closed/` and deleting `plan.md` and
  `todo.md`.
- If the evidence points to a different owner, stop and request a lifecycle
  split instead of broadening idea 302.

Completion check:

- Either `todo.md` records a fresh semantic repair proof for the focused family
  or the lifecycle state has no active `plan.md` / `todo.md` with idea 302
  archived under `ideas/closed/`.

### Step 3: Broader Backend Guard Or Handoff

Goal: separate this owner from remaining backend-regex inventory buckets.

Primary target: focused proof plus broader backend-regex proof if Step 2
modified shared backend code

Actions:

- Record fresh build proof and focused c-testsuite backend proof for `00064`,
  `00139`, and `00205`.
- Run or verify the supervisor-selected broader backend guard when the slice
  touches shared backend lowering or printer code.
- Record remaining residual buckets separately and keep umbrella idea 295 as
  the place for future classification.

Completion check:

- `todo.md` records the focused-family result, broader guard status when
  required, and any remaining non-302 residual bucket without silently
  expanding this owner.
