# RV64 Aggregate Local Subobject And Byval Flow Plan

Status: Active
Source Idea: ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md

## Purpose

Repair the remaining RV64 prepared aggregate-local runtime failures after
generic local frame-slot address materialization has already been handled.

## Goal

Make aggregate-local subobject stores/loads and supported aggregate copy/byval
flows execute correctly on RV64, or reclassify narrower ABI residuals with
emitted-code evidence.

## Core Rule

Do not special-case c-testsuite filenames, struct/union names, or fixed field
offsets. Progress must come from aggregate layout, local memory, and RV64
prepared emission rules.

## Read First

- ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md
- idea 312 closure notes, only if needed to distinguish already-completed
  frame-slot address publication from aggregate-subobject residuals
- Current RV64 prepared local-address, aggregate layout, memory, copy, and call
  lowering code under `src/backend/mir/`
- Existing backend coverage for RV64 local memory, aggregate, byval, and qemu
  runtime behavior under `tests/backend/`

## Scope

- Aggregate-local subobject address and value flow for structs and unions.
- Nested aggregate field offset stores and loads on RV64.
- Aggregate copy and byval/local aggregate transfer when it is the first bad
  runtime mechanism.
- Focused backend tests for semantic aggregate-local behavior.
- Representative c-testsuite probes for `src/00019.c`, `src/00046.c`, and
  `src/00140.c`.

## Non-Goals

- Reworking generic local frame-slot address publication from idea 312.
- Function pointer storage, return, or indirect-call policy.
- Broad vararg or floating aggregate ABI repair unless required to classify a
  narrow aggregate-local proof.
- Global aggregate storage or global address materialization.
- Expectation rewrites, unsupported downgrades, or skipped qemu checks claimed
  as aggregate-local progress.

## Working Model

The selected residuals already emit and link, so treat this as a runtime
semantic failure in prepared aggregate-local lowering. First separate nested
subobject address/value movement from aggregate copy/byval and ABI issues,
then repair the smallest semantic mechanism that explains the focused proof.

## Execution Rules

- Start from emitted-code evidence for `src/00019.c`, `src/00046.c`, and
  `src/00140.c` before changing lowering.
- Add focused backend tests that describe aggregate-local behavior, not source
  filenames.
- Keep local frame-slot base publication stable unless a narrow aggregate proof
  shows it is still incomplete.
- If `src/00140.c` exposes vararg or floating aggregate ABI behavior before an
  aggregate-copy mechanism can be proven, classify that residual instead of
  folding broad ABI repair into this route.
- Use the supervisor-selected proof command for each executor packet and write
  results into `test_after.log` unless delegated otherwise.

## Step 1: Normalize Aggregate Residual Evidence

Goal: classify the three candidate failures by first bad runtime mechanism.

Primary targets:

- `src/00019.c`
- `src/00046.c`
- `src/00140.c`

Actions:

- Reprobe each candidate for emit, link, and qemu outcome.
- Capture emitted RV64 assembly and any available prepared BIR dumps for the
  failing regions.
- Identify whether each failure is caused by nested aggregate subobject
  store/load flow, aggregate copy/byval transfer, or a separate ABI boundary.
- Record the first repair boundary and the focused backend tests to add.

Completion check:

- `todo.md` names the first repair boundary, assigns each candidate to a
  mechanism bucket, and identifies at least one focused test target.

## Step 2: Add Focused Aggregate-Local Coverage

Goal: encode the failing semantic behavior before changing RV64 lowering.

Primary targets:

- Backend tests for nested struct/union local subobject stores and loads.
- Backend tests for aggregate copy/byval transfer only where the evidence shows
  it is currently the first repairable mechanism.

Actions:

- Add coverage for nested aggregate field offset stores and loads through local
  memory.
- Add coverage for aggregate self-pointer or chained aggregate pointer access
  when it isolates the `src/00019.c` mechanism.
- Add aggregate copy/byval coverage only within the narrow supported surface
  identified in Step 1.
- Ensure tests would fail for stale values, missing subobject stores, or fake
  copies rather than only checking assembly text.

Completion check:

- Focused tests expose the current aggregate-local gap and are not tied to
  c-testsuite filenames, fixed offsets, or field names.

## Step 3: Repair Nested Aggregate Subobject Flow

Goal: make RV64 prepared lowering preserve nested aggregate-local stores and
loads through the correct local memory offsets.

Primary targets:

- Aggregate layout and subobject addressing helpers.
- RV64 prepared local memory load/store emission.
- Any prepared value materialization needed for nested aggregate fields.

Actions:

- Inspect how aggregate field offsets and local object bases reach RV64
  prepared emission.
- Repair subobject store/load lowering so nested struct/union fields use the
  intended local memory address and width.
- Preserve existing scalar, pointer, and local frame-slot behavior.
- Re-run focused tests plus representative probes for `src/00019.c` and
  `src/00046.c`.

Completion check:

- Nested aggregate-local focused tests pass, and `src/00019.c`/`src/00046.c`
  either run under qemu or produce a newly classified non-subobject residual
  with emitted-code evidence.

## Step 4: Repair Or Classify Aggregate Copy And Byval Flow

Goal: handle aggregate copy/byval transfer only to the extent supported by the
source idea and current evidence.

Primary target:

- `src/00140.c` and the focused aggregate copy/byval tests from Step 2.

Actions:

- Inspect whether the first bad mechanism is aggregate payload copy, byval
  argument transfer, vararg/floating aggregate ABI behavior, or another
  separate boundary.
- Repair narrow aggregate copy/local transfer when it is the first bad
  mechanism.
- If broad ABI behavior blocks the representative, classify it with emitted
  code and do not implement a broad ABI rewrite inside this route.
- Re-run the focused copy/byval proof and `src/00140.c`.

Completion check:

- Supported aggregate copy/byval tests pass, and `src/00140.c` either runs
  under qemu or is classified as a separate ABI residual with concrete
  evidence.

## Step 5: Reprobe And Close Classification

Goal: prove idea 314 acceptance criteria or identify the next separate source
idea.

Primary targets:

- `src/00019.c`
- `src/00046.c`
- `src/00140.c`
- All focused aggregate-local tests added during this runbook.

Actions:

- Reprobe the candidate set for emit, link, and qemu outcome.
- Summarize which repairs landed and which residuals remain outside
  aggregate-local subobject/byval flow.
- Add follow-up source ideas only for separate mechanisms discovered during
  execution.
- Run the supervisor-selected backend guard or broader validation checkpoint.

Completion check:

- Focused backend coverage passes, representative candidates are either qemu
  passes or classified with concrete emitted-code evidence, and no progress
  depends on testcase-shaped matching or weakened expectations.
