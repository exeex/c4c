# RV64 Scalar/FPR Residual Salvage Runbook

Status: Active
Source Idea: ideas/open/550_rv64_scalar_fpr_residual_salvage.md

## Purpose

Classify the current low-volume RV64 scalar/FPR/helper residual rows and only
split or implement follow-up work when the row evidence identifies a coherent
non-F128 semantic owner.

## Goal

Turn the six recorded residual rows into owned, screened evidence packets, then
create narrow follow-up ideas or stop with a durable no-implementation record.

## Core Rule

Do not let low-volume residual salvage displace higher-impact RV64 recovery or
merge into F128/long-double helper work.

## Read First

- ideas/open/550_rv64_scalar_fpr_residual_salvage.md
- Current failure evidence or bucket maps that identify the six residual rows
- Existing RV64 backend and BIR tests for scalar compare publication,
  floating casts, and variadic helper lowering

## Scope

- Three `unsupported_scalar_compare_publication` rows
- Two `unsupported_floating_cast` rows
- One `unsupported_variadic_helper_lowering` row
- First-owner classification and F128 screening for each row
- Narrow follow-up idea creation only when a residual group has coherent facts

## Non-Goals

- Do not repair F128 arithmetic, F128 conversion, long-double helpers, or
  external soft-float support.
- Do not rewrite broad FPR or call ABI paths from the six residual rows alone.
- Do not change expected outputs, unsupported markers, or pass/fail accounting
  as a substitute for capability work.
- Do not mix variadic helper, scalar compare, and floating-cast work into one
  implementation slice unless evidence proves one shared semantic owner.

## Working Model

Treat this as a salvage/classification lane first. The useful output may be a
small set of precise follow-up ideas rather than immediate implementation.
Implementation is allowed only after a group has current row evidence, F128
screening, a first owner, and a narrow proof surface.

## Execution Rules

- Preserve the source idea unless durable intent genuinely changes.
- Record packet progress and proof in `todo.md`.
- Use route artifacts under `build/agent_state/550_rv64_scalar_fpr_residual_salvage/`
  for case-specific evidence.
- Prefer semantic owner classification over testcase-shaped matching.
- If a row proves F128/long-double-related, quarantine it away from this lane.
- If a residual group is coherent enough for implementation, create a separate
  `ideas/open/*.md` follow-up before switching lifecycle state.

## Ordered Steps

### Step 1: Reproduce Current Residual Rows

Goal: identify the exact current residual rows for the scalar compare,
floating-cast, and variadic helper categories.

Actions:
- Locate current evidence for the three scalar compare rows, two floating-cast
  rows, and one variadic helper row.
- Rerun representative routes only where current evidence is stale or missing.
- Save commands, return codes, and logs under the Step 1 agent-state directory.
- Record whether the six-row inventory still matches current behavior.

Completion check:
- `todo.md` lists all current residual rows or records which rows disappeared.
- Each retained row has a route artifact or a clearly referenced current
  evidence artifact.

### Step 2: Screen For F128 And Long-Double Leakage

Goal: separate ordinary-C scalar/FPR/helper residuals from F128 or long-double
work that belongs to another lane.

Actions:
- Inspect types, helpers, and prepared facts for every retained row.
- Mark F128, long-double, or soft-float helper involvement explicitly.
- Exclude quarantined rows from scalar/FPR salvage implementation decisions.

Completion check:
- `todo.md` records an F128/long-double screening result for each row.
- No quarantined row is used as justification for this lane's implementation.

### Step 3: Classify First Semantic Owners

Goal: determine whether scalar compare publication, floating casts, and
variadic helper lowering share any implementation owner.

Actions:
- Classify each retained, non-quarantined row by first failing semantic owner.
- Separate producer, RV64 lowering, helper ABI, and diagnostic-only ownership.
- Compare the three groups for shared ownership before proposing any repair.

Completion check:
- `todo.md` records first-owner facts for each retained row.
- The plan has enough evidence to decide split, implement, or stop.

### Step 4: Split Coherent Follow-Ups Or Record No-Implementation

Goal: keep this source idea as a classification lane unless evidence supports
small, owned implementation follow-ups.

Actions:
- If a group has coherent ownership and proof scope, create a narrow follow-up
  idea under `ideas/open/` with reviewer reject signals.
- If a group lacks coherent current evidence, record why no immediate
  implementation is justified.
- Keep scalar compare, floating-cast, and variadic helper work separate unless
  Step 3 proves a shared owner.

Completion check:
- Either narrow follow-up ideas exist for each actionable group, or `todo.md`
  records why no implementation should proceed now.
- No implementation code is changed as part of this classification step.

### Step 5: Closure Readiness Check

Goal: decide whether the source idea is complete as a classification/splitting
lane.

Actions:
- Confirm every retained row has reproduction status, F128 screening, and first
  owner classification.
- Confirm actionable work has been split into separate open ideas, not silently
  absorbed into this plan.
- Run no broad validation unless implementation code was changed elsewhere;
  lifecycle-only closure should use the normal close gate.

Completion check:
- The source idea's acceptance criteria are satisfied or the remaining gap is
  clearly blocked in `todo.md`.
- The supervisor can call plan-owner closure evaluation without re-deriving the
  classification state.
