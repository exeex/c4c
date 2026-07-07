# RV64 Pointer Arithmetic Result Publication Runbook

Status: Active
Source Idea: ideas/open/583_rv64_pointer_arithmetic_result_publication.md

## Purpose

Repair RV64 object emission for pointer-valued BIR add/sub results where a
prepared pointer base is combined with an integer byte offset, then publish the
result so later memory users can consume the prepared owner.

## Goal

Advance the retained `src/20000819-1.c` pointer arithmetic representative past
the current `unsupported_pointer_arithmetic` owner, or expose a distinct
downstream owner after semantic pointer-result publication.

## Core Rule

Implement the prepared pointer add/sub lowering contract; do not match
filenames, functions, blocks, value names, diagnostics, or one representative
source shape.

## Read First

- `ideas/open/583_rv64_pointer_arithmetic_result_publication.md`
- `ideas/closed/575_rv64_pointer_arithmetic_lowering.md`
- RV64 prepared scalar/object emission code for pointer-valued binary
  operations
- Focused RV64 backend object-emission tests for prepared pointer homes and
  local-memory consumers

## Current Scope

- RV64 object emission for pointer-valued BIR `add`/`sub` with a prepared
  pointer base plus integer byte offset.
- Publication of the resulting pointer value into the prepared destination
  home expected by later memory operations.
- Focused object-emission coverage for loaded-base plus scaled-offset pointer
  arithmetic, including frame/register destination homes when current evidence
  justifies them.
- Representative route proof for `src/20000819-1.c`.
- Precise fail-closed diagnostics for unsupported pointer arithmetic operand
  types, homes, or prepared facts.

## Non-Goals

- Reconstructing address provenance from raw source text, LIR, or
  target-shaped instruction fragments.
- Broad local-memory, select, call, inline asm, floating-point, variadic
  helper, F128, or unrelated integer ALU rewrites.
- Expectation rewrites, unsupported-marker changes, allowlist edits, or
  runtime-comparison changes as a substitute for capability repair.
- Renaming the old pointer arithmetic failure without publishing the pointer
  result to the prepared destination home.

## Working Model

Idea 575 narrowed pointer arithmetic classification but did not repair the
capability. The retained evidence says prepared BIR already exposes the
pointer-valued binary instruction, loaded pointer base, scaled integer byte
offset, and destination owner. The remaining gap is late RV64 materialization
and publication of the pointer result so downstream memory operations observe a
prepared pointer home instead of the same owner failure.

## Execution Rules

- Keep changes inside the RV64 object-lowering boundary for pointer-result
  address arithmetic unless Step 1 proves a narrower owner.
- Add semantic focused tests before or alongside lowering changes; avoid
  representative-shaped shortcuts.
- Preserve fail-closed behavior for unsupported pointer arithmetic forms with
  a narrower diagnostic.
- Use `todo.md` for packet progress and proof notes; do not rewrite this
  runbook for routine executor updates.
- Backend proof must include a fresh build plus the focused RV64 backend bucket
  touched by the implementation. Escalate to `^backend_` before closure.

## Step 1: Reproduce Pointer Arithmetic Owner

Goal: Confirm the current pointer arithmetic owner and prepared operand/home
shape for the retained representative.

Actions:
- Build `c4cll`.
- Regenerate prepared dumps and RV64 object-route logs for `src/20000819-1.c`.
- Record the pointer-valued BIR add/sub instruction, pointer base home, integer
  byte-offset source, destination owner/home, and current diagnostic.
- Identify the code path that reports `unsupported_pointer_arithmetic`.

Completion check:
- `todo.md` records the current diagnostic, route coordinates when available,
  prepared pointer arithmetic facts, and the focused proof command/log.

## Step 2: Add Focused Pointer Publication Coverage

Goal: Pin the semantic pointer-result publication contract in backend tests.

Actions:
- Add or adjust focused RV64 object-emission tests that build pointer-valued
  add/sub from a prepared pointer base plus integer byte offset.
- Cover the destination publication shape required by later memory consumers.
- Preserve precise unsupported diagnostics for non-lowerable pointer
  arithmetic forms.
- Avoid expectations tied to `src/20000819-1.c`, `foo`, block names, value
  names, or exact diagnostic strings beyond the intended fail-closed contract.

Completion check:
- The focused backend test fails for the intended missing publication path
  before repair, or passes with a precise fail-closed diagnostic contract, and
  `todo.md` records the proof.

## Step 3: Repair Pointer Result Materialization

Goal: Materialize supported pointer add/sub results and publish them into the
prepared destination home.

Actions:
- Locate the RV64 prepared emission path that owns pointer-valued binary
  arithmetic.
- Lower supported pointer base plus byte-offset add/sub using existing RV64
  address, register, and move/publish helper machinery.
- Publish the pointer result to the prepared destination home so later memory
  operations consume the produced owner.
- Keep unsupported operand types, homes, and missing prepared facts fail-closed
  under a pointer-arithmetic-specific diagnostic.

Completion check:
- Focused RV64 backend coverage passes and proves pointer-result
  materialization plus destination publication for the supported shape.

## Step 4: Representative Route Proof

Goal: Prove the retained representative advances past the old pointer
arithmetic owner or lands on a distinct downstream owner.

Actions:
- Rebuild `c4cll`.
- Rerun prepared dumps and RV64 object-route logs for `src/20000819-1.c`.
- Compare against Step 1 and record the new owner, downstream diagnostic, or
  successful object emission state.
- Confirm the proof does not depend on unrelated local-memory, select, call, or
  integer ALU rewrites.

Completion check:
- `todo.md` records that the loaded-base plus scaled-offset representative no
  longer stops at the old `unsupported_pointer_arithmetic` owner, or records a
  distinct downstream owner with concrete route evidence.

## Step 5: Backend Closure Readiness

Goal: Establish that the source idea is ready for lifecycle closure evaluation.

Actions:
- Run the focused backend object-emission test.
- Run the broader backend subset chosen by the supervisor, normally
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Confirm focused coverage, representative route proof, destination
  publication, and source idea acceptance criteria are all satisfied.

Completion check:
- `todo.md` records fresh backend proof and explicitly states whether the
  source idea is ready for plan-owner closure evaluation.
