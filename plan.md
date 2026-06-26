# RV64 gcc_torture Prepared Module Shape Classification Runbook

Status: Active
Source Idea: ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md
Activated from: ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md

## Purpose

Finish the umbrella lifecycle work for RV64 gcc_torture prepared-module-shape
classification now that the later RV64 object-route child ideas have closed.

## Goal

Decide whether idea 354's classification umbrella can close, or identify the
exact remaining unclassified prepared-module-shape bucket and split it to a
separate open idea.

## Core Rule

This is an analysis and lifecycle umbrella, not an implementation plan. Do not
repair RV64 lowering directly from this runbook; create or activate a focused
child idea if fresh scan evidence exposes a repairable backend family.

## Read First

- `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
- `review/354_prepared_shape_classification.md`
- recently closed child ideas under `ideas/closed/`:
  - `355_rv64_prepared_object_shape_diagnostics.md`
  - `356_rv64_object_route_assembler_backed_prepared_text.md`
  - `357_rv64_object_route_data_sections_globals_strings.md`
  - `358_rv64_object_route_abi_width_edges.md`
  - `359_bir_prepared_object_consumer_contract_completion.md`
  - `368_rv64_object_route_frame_slot_base_offset_memory.md`
  - `369_rv64_object_route_terminator_fragment_lowering.md`
  - `370_rv64_object_route_byval_aggregate_param_homes.md`
  - `371_rv64_object_route_aggregate_va_arg_helper_lowering.md`
  - `372_rv64_object_route_frame_slot_address_call_args.md`
  - `373_rv64_object_route_frame_slot_value_call_args.md`
  - `374_rv64_object_route_non_register_param_homes.md`
  - `375_rv64_object_route_scalar_compare_trunc_lowering.md`
  - `383_rv64_global_aggregate_lane_materialization.md`
  - `384_prepared_global_symbol_memory_access_publication.md`
  - `386_rv64_object_route_same_module_byval_aggregate_call_args.md`
  - `387_rv64_object_route_same_module_sret_calls.md`
  - `391_rv64_variadic_prologue_save_area_publication.md`
  - `392_rv64_va_list_expression_call_argument_value_publication.md`
  - `393_rv64_variadic_aggregate_va_arg_cursor_stride.md`
  - `394_rv64_same_module_sret_callee_home_publication.md`
- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` if present
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` if present

## Current Targets

- Final disposition of umbrella idea 354.
- Open/closed child coverage for the prepared-module-shape failure family.
- Fresh or existing RV64 gcc_torture backend scan evidence.
- Residual failure buckets, if any, that still lack an open lifecycle owner.

## Non-Goals

- Do not implement RV64 object-route repairs in this umbrella.
- Do not weaken the RV64 backend object runner or mark cases unsupported to
  improve scan counts.
- Do not reopen closed child ideas without fresh contradictory evidence.
- Do not mix the unrelated EV64 `.insn.d` length-prefix idea 385 into this
  classification umbrella.
- Do not create testcase-shaped child ideas; any new child must name the
  semantic backend family and reject overfit routes.

## Working Model

Idea 354 originally classified the dominant RV64 gcc_torture prepared-module
shape failures and spawned child ideas for the repairable buckets. Later
refreshes found residual structured buckets and spawned additional children.
The currently visible `ideas/open/` inventory contains only this umbrella and
unrelated idea 385, so the next task is to verify whether the child set now
covers or closed the dominant prepared-module-shape failures.

## Execution Rules

- Prefer existing scan artifacts when they are current enough to answer the
  closure question.
- If scan artifacts are stale or missing, run a representative or full
  classifier step before deciding closure.
- Record all transient analysis in `todo.md` or `review/`; only update the
  source idea when closing, deactivating, or splitting a new durable initiative.
- Before closing 354, run the plan-owner close gate required by lifecycle
  rules.

## Steps

### Step 1: Audit Child Idea Disposition

Goal: verify that all child ideas required by 354 are closed or intentionally
superseded.

Primary target: `ideas/open/`, `ideas/closed/`, and the child lists in idea
354.

Actions:

- Enumerate every child idea named by idea 354.
- Confirm each child is closed, superseded, or still open.
- Record any missing or still-open child owner in `todo.md`.
- Confirm that idea 385 is unrelated to the RV64 gcc_torture prepared-shape
  umbrella.

Completion check: `todo.md` contains a child-disposition table or concise
audit summary and names any blocker to closing 354.

### Step 2: Refresh Or Validate Classification Evidence

Goal: decide whether existing scan/classification evidence is current enough
for a closure decision.

Primary target: RV64 gcc_torture backend scan artifacts and
`review/354_prepared_shape_classification.md`.

Actions:

- Inspect current scan summaries and representative refresh logs.
- If needed, rerun the RV64 gcc_torture backend scan or an explicitly
  documented representative subset.
- Compare residual failures with the closed child set.
- Record counts, representative cases, and any unowned bucket in `todo.md`.

Completion check: `todo.md` states whether the prepared-module-shape family is
fully classified or identifies the exact residual unowned bucket.

### Step 3: Split Any Residual Owner

Goal: keep implementation work out of the umbrella if a fresh residual backend
family remains.

Primary target: `ideas/open/` only if Step 2 finds unowned repair work.

Actions:

- For each repairable residual bucket without an owner, create a focused child
  idea under `ideas/open/`.
- Include representative cases, expected backend stage, proof command, and
  concrete reviewer reject signals.
- If residual failures are non-backend or not actionable, document why they do
  not block 354 closure.
- Do not edit implementation files.

Completion check: every residual bucket is either covered by a focused child
idea or documented as non-blocking/non-actionable.

### Step 4: Decide Umbrella Closure

Goal: close idea 354 if the source acceptance criteria are satisfied.

Primary target: lifecycle files only.

Actions:

- Verify all source acceptance criteria in idea 354.
- Run the plan-owner-required regression close gate using the accepted current
  baseline/log scope.
- If acceptance is satisfied, move idea 354 to `ideas/closed/` with closure
  notes and remove `plan.md`/`todo.md`.
- If acceptance is not satisfied, leave a coherent active plan and `todo.md`
  with the exact remaining blocker.

Completion check: lifecycle state is coherent: either 354 is closed and no
active plan remains, or `plan.md`/`todo.md` remain active with the next
executable classification step.
