# Prepared Object-Data Zero-Fill Contract Runbook

Status: Active
Source Idea: ideas/open/562_prepared_object_data_zero_fill_contract.md

## Purpose

Repair the prepared object-data producer contract for ordinary
zero-initialized global objects so RV64 object emission receives coherent
zero-fill or byte-data authority instead of `unsupported_but_coherent`.

## Goal

Move `src/20000412-1.c` off the prepared selected object-data contract
diagnostic by publishing semantic prepared zero-fill authority for the full
global object extent, with focused tests that do not depend on the filename or
the exact 1656-byte representative size.

## Core Rule

Fix the prepared object-data producer boundary. Do not teach RV64 object
emission to accept missing selected object data, and do not change
expectations, unsupported markers, allowlists, or pass/fail accounting as
progress.

## Read First

- `ideas/open/562_prepared_object_data_zero_fill_contract.md`
- `build/agent_state/548_step2_global_data_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20000412-1.c/case.log`
- Prepared/global data production code under `src/backend/prepared/`
- RV64 object-data diagnostics under `src/backend/mir/riscv/codegen/`
- Existing backend tests that assert prepared or object-data facts

## Scope

- Ordinary zero-initialized global arrays and objects.
- Prepared selected object-data facts for full object extents.
- Focused tests for zero-fill or emitted-byte authority that exercise the
  producer contract directly.
- Representative RV64 proof for `src/20000412-1.c` after focused coverage is
  in place.

## Non-Goals

- Do not implement RV64 consumption of `unsupported_but_coherent` selected
  object data.
- Do not work on F64 global-memory loads, stack-frame slots, move-bundle
  authority, F128, or long-double routes.
- Do not revisit closed BIR global shape handoff, runtime intrinsic memory, or
  call metadata producer ideas.
- Do not special-case `src/20000412-1.c`, object label ids, or a 1656-byte
  extent.

## Working Model

The representative already has enough object identity and extent for prepared
or RV64 code to name the object, but selected object data is reported as
`unsupported_but_coherent` with both emitted-byte and zero-fill counts at zero.
The first repair target is the prepared data producer that decides whether a
zero-initialized object has coherent selected data for its full extent.

## Execution Rules

- Start by confirming the current diagnostic and exact producer/consumer
  boundary for the representative.
- Add focused tests before or with the repair so the producer contract is
  checked semantically.
- Keep RV64 changes limited to diagnostic observation unless inspection proves
  the selected prepared data is already coherent.
- Preserve fail-closed behavior for genuinely unsupported object data.
- Record proof commands, log paths, and any residual downstream owner in
  `todo.md`.

## Steps

### Step 1: Inspect Zero-Fill Object-Data Boundary

Goal: Reconfirm the current first bad fact for `src/20000412-1.c` and locate
the producer gate that leaves selected object data without zero-fill authority.

Primary target:

- `src/20000412-1.c`

Actions:

- Re-run or inspect the representative backend log and capture the current
  `unsupported_global_data` diagnostic.
- Trace the selected object-data status from prepared/global-data production to
  RV64 object emission.
- Identify the minimal producer-owned function or helper that should publish
  zero-fill or emitted-byte authority for an ordinary zero-initialized global
  object.
- Record whether the boundary is still prepared producer-owned or has moved to
  RV64 consumption.

Completion check:

- `todo.md` records the current diagnostic, log paths, owning code boundary,
  and a concrete next packet for focused coverage or repair.

### Step 2: Add Focused Zero-Fill Contract Coverage

Goal: Add tests that prove prepared selected object data carries coherent
zero-fill authority for ordinary zero-initialized globals.

Actions:

- Add focused prepared/backend coverage for zero-initialized global arrays or
  objects using semantic facts, not filename or object-size matching.
- Cover the representative shape narrowly enough to prevent regressions while
  avoiding a one-case shortcut.
- Keep expected failure behavior for unsupported data unchanged.

Completion check:

- Focused tests fail before the repair or directly prove the repaired producer
  contract, and backend test proof is recorded in `todo.md`.

### Step 3: Repair Prepared Zero-Fill Publication

Goal: Publish coherent selected object data for ordinary zero-initialized
global objects at the prepared producer boundary.

Actions:

- Implement the minimal prepared producer repair that publishes full-extent
  zero-fill or byte-data authority.
- Avoid RV64 inference of missing data and avoid broad object-emission rewrites.
- Preserve existing behavior for nonzero initializers, external declarations,
  unsupported relocations, and unsupported object-data shapes.
- Run focused tests and the backend subset selected by the supervisor.

Completion check:

- Backend tests pass, focused zero-fill coverage passes, and `todo.md` records
  the files changed plus proof commands.

### Step 4: Reconcile Representative And Residual Owner

Goal: Prove whether `src/20000412-1.c` moved off the prepared selected
object-data contract diagnostic and classify any later failure.

Actions:

- Re-run the representative allowlist with verbose failure logs.
- Confirm the old diagnostic no longer appears.
- If the row advances to a downstream owner, record the exact owner and leave
  that work to a separate source idea.
- Recommend close, split, or continue for this source idea.

Completion check:

- `todo.md` records representative outcome, log paths, residual owner if any,
  and the lifecycle recommendation for this active idea.
