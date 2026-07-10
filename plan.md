# RISC-V Object Emission Internal Probe Runbook

Status: Active
Source Idea: ideas/open/664_riscv_object_emission_internal_probe.md

## Purpose

Finish the lifecycle for the RISC-V object-emission infrastructure row without
absorbing unrelated RV64 runtime, CLI, byval, or dump-contract work.

Goal: refresh the current `backend_riscv_object_emission` boundary, confirm
whether the post-664 regression split work has cleared the old closure blocker,
and prepare an evidence-backed lifecycle decision for idea 664.

## Core Rule

Keep this runbook scoped to row 256 `backend_riscv_object_emission` and its
direct object-emission infrastructure contract. Do not reopen unrelated backend
families unless focused evidence proves row 256 currently shares their first
owner.

## Read First

- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `ideas/closed/673_post_664_full_suite_regression_probe.md`
- `ideas/closed/674_rv64_object_terminator_lowering.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- Current focused row-256 logs and any supervisor-selected regression logs.

## Current Targets

- `backend_riscv_object_emission`
- Existing focused evidence under
  `build/agent_state/664_step4_local_frame_address_publication/`
- Post-664 split context from ideas 673 and 674, only as closure-blocker
  evidence.

## Non-Goals

- Do not implement RV64 pointer-local, byval, destination-publication,
  object-data static storage, callee-saved GPR, packed-member, AArch64, CLI,
  LLVM torture, or dump-contract repairs.
- Do not change expectations, unsupported markers, allowlists, timeout
  behavior, runtime policy, or baseline accounting.
- Do not claim progress from diagnostic wording, helper renames,
  classification-only changes, or final object bytes without naming the object
  contract.
- Do not edit transient review artifacts.

## Working Model

Idea 664 originally owned the focused row-256 RISC-V object-emission probe.
Its lifecycle note records that row 256 moved from failed to passed, but close
was rejected because a later full-suite candidate exposed rows 139 and 176.
Those rows were split and closed through ideas 673 and 674. Execution should
verify the current row-256 boundary and decide whether idea 664 is now ready
for lifecycle closure or still has a precise in-scope blocker.

## Execution Rules

- Treat row 256 as the only primary implementation target.
- Use the closed 673/674 records only to determine whether the old close
  blocker remains relevant.
- If current evidence shows a new first owner outside row-256 object emission,
  record it as a separate follow-up instead of expanding this runbook.
- Keep routine progress and proof notes in `todo.md`.
- Any code-changing packet must include fresh build or compile proof plus the
  supervisor-selected focused CTest subset.

## Step 1: Refresh Row 256 Boundary

Goal: establish the current `backend_riscv_object_emission` status and first
observable boundary from fresh focused evidence.

Primary target:
`backend_riscv_object_emission`

Actions:

- Inspect the source idea lifecycle note and prior focused row-256 evidence.
- Run the supervisor-selected focused row-256 proof command, or the narrowest
  equivalent focused CTest command if no command is delegated.
- Record whether row 256 passes, fails closed with a precise diagnostic, or
  exposes a current object-emission owner.
- Do not inspect or repair unrelated backend families unless row 256 evidence
  directly requires it.

Completion check:

- `todo.md` records the current row-256 command, result, and first boundary,
  and names whether Step 2 should proceed as closure reconciliation or focused
  repair.

## Step 2: Reconcile Post-664 Split Evidence

Goal: decide whether the old closure blocker from rows 139 and 176 is still
relevant to idea 664.

Actions:

- Read the closure notes for ideas 673 and 674.
- Compare their accepted proof surfaces with the refreshed row-256 status.
- If rows 139 or 176 currently regress row 256, record the exact shared
  object-emission contract before proposing any repair.
- If the split work fully isolates or resolves the blocker, keep it as closure
  evidence and do not expand idea 664.

Completion check:

- `todo.md` states whether the post-664 blocker is cleared, still blocks
  closure, or requires a separate new open idea.

## Step 3: Prove Regression Safety And Lifecycle Readiness

Goal: provide enough proof for the plan owner to close idea 664 or reject close
with a precise in-scope blocker.

Actions:

- Run the supervisor-selected regression guard scope for row 256 and any
  directly linked post-664 split rows.
- Preserve canonical `test_before.log` and `test_after.log` only under the
  supervisor's delegated proof policy.
- Confirm no implementation, expectation, unsupported-marker, allowlist,
  timeout, runtime-policy, baseline-accounting, or transient review files were
  changed by this lifecycle activation.
- Hand the result to the plan owner for close, reject, or split.

Completion check:

- `todo.md` contains the close-readiness proof command and result, and the
  source idea can be closed if row 256 remains passing with no new in-scope
  regression.
