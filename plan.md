# BIR Bootstrap Global Data-Shape Handoff Support Runbook

Status: Active
Source Idea: ideas/open/561_bir_bootstrap_global_data_shape_handoff_support.md

## Purpose

Repair BIR bootstrap and global data-shape handoff support for the related
rows that currently fail before prepared/RV64 object lowering can safely
consume global-data facts.

## Goal

Publish coherent BIR handoff facts for supported global data shapes so
representative RV64 rows advance because the bootstrap/global data model is
available, not because prepared or RV64 consumers infer missing facts.

## Core Rule

Keep this as BIR bootstrap/global data-shape handoff work. Do not count these
rows as exact `semantic lir_to_bir` producer rows, do not bypass the handoff
limitation in prepared/RV64 lowering, and do not use expectation rewrites,
unsupported-marker downgrades, or filename-shaped shortcuts as progress.

## Read First

- `ideas/open/561_bir_bootstrap_global_data_shape_handoff_support.md`
- BIR bootstrap and global-data handling under `src/backend/bir/`
- Existing focused BIR note tests in `tests/backend/bir/`
- Representative RV64 rows from the related global data-shape bucket,
  including `tests/c/external/gcc_torture/src/strlen-2.c` or a current
  stronger representative

## Scope

- Inspect current bootstrap/global data-shape rows before prepared object
  handoff.
- Add focused coverage for scalar integer/pointer globals, linear integer
  array globals, and aggregate-backed globals with byte-address semantics when
  they are retained in this lane.
- Repair the BIR handoff limitation without merging this work into local
  memory, call metadata, runtime/intrinsic, scalar/signature/control, prepared,
  or RV64 object-lowering lanes.
- Prove representative RV64 rows advance through BIR handoff and classify any
  remaining failures as downstream ownership.

## Non-Goals

- Do not treat these `44` related rows as part of the exact semantic producer
  row count.
- Do not implement prepared global-data layout, RV64 object-route global
  lowering, stack-frame support, or move-bundle classification unless a later
  source idea owns that work.
- Do not re-open closed local-memory, call-metadata, or runtime/intrinsic
  producer lanes.
- Do not fold F128-specific global data requirements into this ordinary-C
  handoff route.
- Do not change expected outputs, unsupported markers, allowlists, or pass/fail
  accounting to claim capability progress.

## Working Model

The related rows are expected to reach a BIR bootstrap/global data-shape
boundary where supported source-level globals are not represented with the
facts needed by the prepared object handoff. The first route should reconstruct
the current row shapes, identify which global forms share the same handoff
limitation, then repair only the shared BIR boundary that the evidence
supports.

## Execution Rules

- Start with current evidence; do not rely on stale row counts or old bucket
  labels without a fresh representative log.
- Keep each implementation slice tied to one BIR handoff fact and focused
  backend coverage.
- Split the source idea before implementation if scalar globals, linear arrays,
  and aggregate-backed byte-address globals do not share a coherent handoff
  owner.
- Preserve the distinction between BIR handoff facts and prepared/RV64
  consumption. If BIR facts are already complete, stop and route downstream
  work to a separate idea.
- Use `test_after.log` for executor backend proof unless the supervisor
  delegates another canonical artifact.
- Escalate validation when a helper change affects multiple global-data
  shapes or shared bootstrap behavior.

## Steps

### Step 1: Inspect Bootstrap Global Data-Shape Boundary

Goal: Reconstruct the current BIR handoff failure boundary for representative
global data-shape rows.

Primary targets:

- `tests/c/external/gcc_torture/src/strlen-2.c`
- At least one current scalar integer or pointer global representative
- At least one current linear integer-array or aggregate-backed global
  representative when available

Actions:

- Reproduce a narrow RV64 subset for the selected representatives.
- Capture diagnostics, BIR/bootstrap evidence, and the prepared handoff point
  for each retained row shape.
- Identify whether the missing facts are BIR bootstrap/global data-shape facts
  or downstream prepared/RV64 global-data consumption facts.
- Decide whether the retained row shapes share one BIR handoff boundary or
  require a lifecycle split.

Completion check:

- `todo.md` records representative rows, log paths, the exact first missing
  handoff fact, likely owning files, and whether Step 2 can add focused shared
  BIR coverage.

### Step 2: Add Focused Global Data-Shape Handoff Coverage

Goal: Lock down the desired BIR handoff facts for the global shapes retained
after Step 1.

Actions:

- Add focused BIR tests for scalar integer and/or pointer globals when Step 1
  proves they are part of the retained handoff boundary.
- Add focused BIR tests for linear integer-array globals when Step 1 proves
  they are part of the retained handoff boundary.
- Add focused BIR tests for aggregate-backed globals with byte-address
  semantics when Step 1 proves they are part of the retained handoff boundary.
- Keep assertions semantic and shape-based; do not encode gcc_torture
  filenames or RV64 target fragments.

Completion check:

- Focused backend tests fail before the repair or are added with the repair in
  the same slice, and each retained global data shape has an explicit expected
  BIR handoff fact.

### Step 3: Repair Shared BIR Global Data-Shape Handoff

Goal: Publish the missing BIR bootstrap/global data-shape facts for the shared
boundary identified in Step 1.

Actions:

- Repair the BIR bootstrap/global-data path identified in Step 1.
- Keep changes inside BIR handoff code and direct helpers unless a shared
  bootstrap helper must change for the retained shapes.
- Preserve fail-closed behavior for unsupported global shapes, F128-primary
  rows, and downstream prepared/RV64 gaps.
- If inspection proves a retained shape has a separate owner, stop and ask the
  plan owner to split rather than broadening the repair.

Completion check:

- Focused backend BIR tests pass.
- Representative RV64 rows no longer fail at the same BIR bootstrap/global
  data-shape handoff boundary.
- Remaining failures are classified as prepared, RV64, F128 quarantine, or
  evidence-gap ownership with log evidence.

### Step 4: Reconcile Bootstrap Global Data-Shape Representatives

Goal: Confirm the retained representatives advanced and decide whether the
source idea is complete or needs a split.

Actions:

- Re-run the narrow RV64 representative subset from Step 1.
- Compare final outcomes against the initial handoff diagnostics.
- Record whether residual failures belong to prepared global-data, RV64 object
  route, stack-frame/move-bundle, F128 quarantine, or evidence-gap work.

Completion check:

- `todo.md` records final representative outcomes and recommends close, split,
  or further runbook work.
