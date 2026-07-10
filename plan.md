# RISC-V Object Emission Internal Probe Runbook

Status: Active
Source Idea: ideas/open/664_riscv_object_emission_internal_probe.md

## Purpose

Activate idea 664 as a probe-first route for the current
`backend_riscv_object_emission` baseline failure. Keep this work at the
RISC-V object-emission infrastructure layer until focused evidence proves a
different first owner.

## Goal

Classify and repair the current RISC-V object-emission failure by naming the
first failing object contract, then applying one general infrastructure repair
only when the probe evidence justifies it.

## Core Rule

Do not absorb unrelated RV64 runtime failures into this route, and do not fix
`backend_riscv_object_emission` by testcase name, final object bytes alone,
expectation churn, unsupported markers, allowlists, timeouts, runtime policy,
or baseline accounting.

## Read First

- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log` if present

## Current Target

- Baseline row: `backend_riscv_object_emission`
- Owning layer: RISC-V object-emission backend infrastructure
- Candidate owners: relocation emission, section layout, symbol publication,
  instruction encoding, or object writer contract

## Non-Goals

- Do not repair RV64 pointer-local, byval, destination-publication,
  object-data static storage, callee-saved GPR, packed local member, AArch64,
  prepared CLI, or LLVM torture rows under this plan.
- Do not change test expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline acceptance.
- Do not claim progress from helper renames, diagnostic-only edits, or
  classification-only changes that leave the same object-emission failure.

## Working Model

The backend history triage left `backend_riscv_object_emission` blocked
pending a focused probe. This runbook starts by refreshing evidence from
prepared/RV64 lowering through object writer output, then narrows to exactly
one object-emission infrastructure owner before allowing implementation.

## Execution Rules

- Keep probe evidence and packet summaries under `build/agent_state/664_*`.
- Preserve existing prepared and RV64 lowering contracts unless the probe
  proves they are stale or missing.
- Treat final object bytes as supporting evidence only after the required
  object contract is named.
- For any code-changing step, run `cmake --build --preset default` plus the
  supervisor-selected focused backend proof, recording canonical executor
  proof in `test_after.log` unless delegated otherwise.
- Stop and report route drift if evidence points first to an unrelated runtime
  semantic family rather than object-emission infrastructure.

## Ordered Steps

### Step 1: Refresh Object-Emission Evidence

Goal: Reproduce the focused `backend_riscv_object_emission` failure and map
the observable boundary from prepared/RV64 lowering into object writer output.

Primary target:

- `backend_riscv_object_emission`

Actions:

- Locate the focused test source, baseline row, and current reproduction
  command for `backend_riscv_object_emission`.
- Refresh prepared-BIR, RV64 lowering, assembly/object emission, symbol,
  relocation, section, and object writer diagnostics relevant to the row.
- Record whether prepared and RV64 lowering inputs are valid, stale, or
  missing before the object writer consumes them.
- Write a short evidence summary under
  `build/agent_state/664_step1_object_emission_probe/`.

Completion check:

- Fresh evidence names the first observable failure boundary, or states that
  the row cannot be reproduced with the exact command and artifacts checked.

### Step 2: Select One Object Contract Owner

Goal: Convert Step 1 evidence into one owned repair target or a fail-closed
diagnostic target.

Actions:

- Classify the first owner as relocation emission, section layout, symbol
  publication, instruction encoding, object writer contract, or a proven
  non-object-emission owner.
- Identify the general contract that should hold at that owner and the
  negative states that must remain fail-closed.
- If the first owner is outside RISC-V object emission, stop with a precise
  lifecycle note instead of broadening this plan.
- Update `todo.md` with the chosen owner and suggested executor packet.

Completion check:

- The next implementation packet has exactly one owner, one expected positive
  condition, and at least one fail-closed negative condition.

### Step 3: Repair The Selected Infrastructure Rule

Goal: Implement one general RISC-V object-emission infrastructure repair
backed by the selected owner evidence.

Actions:

- Modify only the code needed for the selected relocation, section, symbol,
  instruction encoding, or object writer contract.
- Add or update focused tests only where they prove the general contract and
  do not key off testcase identity.
- Preserve nearby prepared/RV64 runtime behavior unless Step 1 proved those
  inputs are stale or missing.
- Build and run the supervisor-selected focused backend proof.

Completion check:

- The focused object-emission row passes or fails closed with a precise
  diagnostic at the selected owner, with fresh build/proof recorded.

### Step 4: Prove Regression Safety And Lifecycle Readiness

Goal: Decide whether idea 664 is complete, blocked, or needs a separate
follow-up idea.

Actions:

- Run the supervisor-selected backend regression subset for this slice.
- Compare remaining failures against the selected object-emission owner and
  reject unrelated RV64 runtime absorption.
- Summarize whether source idea acceptance criteria are satisfied.
- If criteria are met, mark the packet complete in `todo.md` and request
  lifecycle close review; otherwise record the exact blocker.

Completion check:

- The next lifecycle action is unambiguous: close idea 664, continue with a
  named object-emission packet, deactivate with a precise blocker, or split a
  separate follow-up idea.

## Completion Criteria

- Focused probe evidence names the first RISC-V object-emission owner.
- Any repair preserves existing prepared and RV64 lowering contracts unless
  proven stale or missing.
- The focused object-emission row passes or fails closed with a precise
  diagnostic at the proven owner.
- Backend regression proof shows no new failures in the supervisor-selected
  subset.
