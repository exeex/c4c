# Post-664 Row 139 Regression Probe Runbook

Status: Active
Source Idea: ideas/open/673_post_664_full_suite_regression_probe.md

## Purpose

Continue idea 673 after Step 1 split the post-664 regression rows by first
owner. This active runbook is now only for row 139.

## Goal

Repair row 139's RV64 object-emission local-memory/direct-global
pointer-publication reload failure and preserve the fixed
`backend_riscv_object_emission` row.

## Core Rule

Do not expand idea 664's row-256 object-emission route. Do not repair row 176
inside this plan; it is split to
`ideas/open/674_rv64_object_terminator_lowering.md`.

## Read First

- `ideas/open/673_post_664_full_suite_regression_probe.md`
- `ideas/open/674_rv64_object_terminator_lowering.md`
- `ideas/open/664_riscv_object_emission_internal_probe.md`
- `review/reviewA.md`
- `build/agent_state/673_step1_regression_probe/summary.md`
- `build/agent_state/664_step4_local_frame_address_publication/summary.md`
- `log/baseline_cbe01007e8524f6e8deb218d0cbd2f007e4e5896.log`

## Current Targets

- Row 139: `backend_cli_riscv64_pointer_global_local_publication`
- Guard row: `backend_riscv_object_emission`

## Non-Goals

- Do not reopen row 256 unless it regresses.
- Do not work row 176 in this active plan.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, or baseline accounting.
- Do not perform broad RV64 runtime, CLI, or object-emission rewrites without a
  named failing contract.

## Working Model

The focused object-emission row is repaired. Step 1 evidence proved row 139
and row 176 have separate first owners. Row 176 was split to idea 674. Row 139
starts from a prepared dump that succeeds, then `--codegen obj` fails with:

`unsupported_local_memory_access: RV64 object route keeps live direct-global local pointer publication reloads fail-closed`

## Execution Rules

- Keep evidence under `build/agent_state/673_*`.
- Preserve Step 1 classification evidence; do not reclassify row 176 here.
- Use canonical `test_before.log` and `test_after.log` for executor proof when
  delegated.
- Preserve `backend_riscv_object_emission` passing in any acceptance proof.

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

- Complete. `todo.md` recorded separate first owners, and row 176 was split to
  `ideas/open/674_rv64_object_terminator_lowering.md`.

### Step 2: Repair Row 139 Direct-Global Local-Memory Publication Reload

Goal: Apply one narrow semantic repair for the row-139 first failing contract.

Actions:

- Inspect RV64 object-emission local-memory handling for live direct-global
  local pointer publication reloads.
- Modify only the owned backend code for this first failing contract.
- Preserve row 256 and any unrelated accepted baseline behavior.
- Run the supervisor-selected focused proof.

Completion check:

- Row 139 no longer fails with
  `unsupported_local_memory_access: RV64 object route keeps live direct-global
  local pointer publication reloads fail-closed`, and row 256 still passes.

### Step 3: Broader Regression Gate

Goal: Decide lifecycle readiness after the focused row-139 repair.

Actions:

- Run the supervisor-selected broader/backend/full-suite proof needed for this
  milestone.
- Compare against the accepted 3386/3397 baseline.
- Record whether row 139 is resolved and row 176 remains split to idea 674.

Completion check:

- The next lifecycle action is unambiguous: close idea 673, switch to idea 674,
  or keep active with a precise row-139 blocker.

## Completion Criteria

- Row 139 is repaired or has a precise blocker.
- Row 176 is split with explicit lifecycle state in idea 674.
- No new regressions are introduced in the supervisor-selected proof subset.
- `backend_riscv_object_emission` remains passing.
