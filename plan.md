# Post-664 Full-Suite Regression Probe Runbook

Status: Active
Source Idea: ideas/open/673_post_664_full_suite_regression_probe.md

## Purpose

Activate idea 673 as the follow-up route for the two full-suite regressions
that blocked idea 664 closure after the focused row-256 object-emission proof
turned green.

## Goal

Classify rows 139 and 176, repair only the first proven owner, and preserve the
fixed `backend_riscv_object_emission` row.

## Core Rule

Do not expand idea 664's row-256 object-emission route. Treat rows 139 and 176
as separate regression evidence until focused probes prove a shared owner.

## Read First

- `ideas/open/673_post_664_full_suite_regression_probe.md`
- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `review/reviewA.md`
- `build/agent_state/664_step4_local_frame_address_publication/summary.md`
- `log/baseline_cbe01007e8524f6e8deb218d0cbd2f007e4e5896.log`

## Current Targets

- Row 139: `backend_cli_riscv64_pointer_global_local_publication`
- Row 176: `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`
- Guard row: `backend_riscv_object_emission`

## Non-Goals

- Do not reopen row 256 unless it regresses.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, or baseline accounting.
- Do not couple rows 139 and 176 into one code repair without evidence that
  they share a first owner.
- Do not perform broad RV64 runtime, CLI, or object-emission rewrites without a
  named failing contract.

## Working Model

The focused object-emission row is repaired. A broader baseline candidate
found two additional failures outside row 256. Supervisor evidence places
those failures before the final sret/local-frame publication patches, so this
runbook starts with reproduction and owner classification rather than assuming
the last patch owns them.

## Execution Rules

- Keep evidence under `build/agent_state/673_*`.
- Run focused row probes before implementation.
- Use canonical `test_before.log` and `test_after.log` for executor proof when
  delegated.
- Preserve `backend_riscv_object_emission` passing in any acceptance proof.
- If rows 139 and 176 have different first owners, split the second row to a
  separate source idea and stop for lifecycle review.

## Ordered Steps

### Step 1: Reproduce And Classify Regression Rows

Goal: Establish current behavior and first-owner evidence for rows 139 and
176.

Primary targets:

- `backend_cli_riscv64_pointer_global_local_publication`
- `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`
- `backend_riscv_object_emission`

Actions:

- Run focused commands for rows 139 and 176 and capture exact failure text.
- Compare against the accepted baseline row list and the current green row-256
  proof.
- Identify whether each row first fails in CLI dump publication, RV64 object
  runtime behavior, prepared lowering, object emission, or stale baseline
  state.
- Record evidence under `build/agent_state/673_step1_regression_probe/`.

Completion check:

- `todo.md` names one repair owner, or records that rows 139 and 176 require a
  split because their first owners differ.

### Step 2: Repair One Proven Owner Or Split

Goal: Apply one narrow semantic repair if Step 1 proves a shared or prioritized
owner, otherwise create the required follow-up idea.

Actions:

- Modify only the owned backend code for the selected first failing contract.
- Preserve row 256 and any unrelated accepted baseline behavior.
- If the unselected row has a different owner, stop after writing a separate
  open idea for that row.
- Run the supervisor-selected focused proof.

Completion check:

- The selected row improves without new focused failures, and row 256 still
  passes.

### Step 3: Broader Regression Gate

Goal: Decide lifecycle readiness after the focused repair or split.

Actions:

- Run the supervisor-selected broader/backend/full-suite proof needed for this
  milestone.
- Compare against the accepted 3386/3397 baseline.
- Record whether rows 139 and 176 are resolved, split, or still blocking.

Completion check:

- The next lifecycle action is unambiguous: close idea 673, switch to a split
  follow-up, or keep active with a precise blocker.

## Completion Criteria

- Rows 139 and 176 are either repaired or split with explicit lifecycle state.
- No new regressions are introduced in the supervisor-selected proof subset.
- `backend_riscv_object_emission` remains passing.
