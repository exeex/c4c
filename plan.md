# RV64 Function Pointer Local And Indirect Call Flow Plan

Status: Active
Source Idea: ideas/open/315_rv64_function_pointer_local_and_indirect_call_flow.md

## Purpose

Repair RV64 prepared lowering for function pointer values that move through
local storage, return values, and indirect-call use.

## Goal

Make function-address values behave as real pointer values on RV64 so focused
local store/load, function-pointer return, and indirect-call cases either run
under qemu or split only after concrete evidence shows a separate residual.

## Core Rule

Do not hard-code candidate filenames, symbol names, field offsets, or direct
call substitutions. Progress must come from semantic function-address value
materialization and indirect-call lowering.

## Read First

- ideas/open/315_rv64_function_pointer_local_and_indirect_call_flow.md
- Idea 312 closure notes only if needed to distinguish local-address residuals
  from function-pointer value residuals
- Current RV64 prepared call, indirect-call, local storage, and symbol-address
  lowering under `src/backend/mir/`
- Existing backend tests for RV64 direct calls, local pointer storage, selected
  indirect calls, and function-symbol address handling under `tests/backend/`

## Scope

- Materializing function addresses as pointer values on RV64.
- Storing and loading function pointers through local scalar or aggregate homes.
- Returning function pointer values consumed by RV64 caller code.
- Indirect call emission through loaded or returned function-pointer values.
- Focused backend tests that prove runtime use through function pointers.
- Representative probes for `src/00087.c` and `src/00124.c`.

## Non-Goals

- General external empty-stub policy.
- Ordinary direct call argument/result lowering already covered by prior RV64
  runtime work.
- Aggregate subobject or byval repair except where needed only to reach a
  function-pointer field in a focused proof.
- Global data object materialization outside function-symbol address handling.
- Rewriting indirect calls into direct calls without preserving
  function-pointer semantics.

## Working Model

Treat `src/00087.c` and `src/00124.c` as one feature family: function symbols
must be materialized as pointer values, those pointer values must survive local
store/load and return flow, and RV64 must emit an indirect call through the
runtime pointer value. A fix for only one candidate is not enough if the same
function-pointer value mechanism remains broken nearby.

## Execution Rules

- Start from emitted-code evidence for both candidates before changing lowering.
- Add focused tests that execute through function pointers, not only assembly
  text checks for one symbol.
- Keep direct-call behavior stable while adding or repairing function-pointer
  value and indirect-call paths.
- If a candidate reaches an aggregate or global-data residual unrelated to
  function-pointer semantics, classify it with concrete evidence instead of
  absorbing that broader mechanism into this route.
- Use the supervisor-selected proof command for each executor packet and write
  results into `test_after.log` unless delegated otherwise.

## Step 1: Normalize Function Pointer Residual Evidence

Goal: classify `src/00087.c` and `src/00124.c` by first bad
function-pointer mechanism.

Primary targets:

- `src/00087.c`
- `src/00124.c`

Actions:

- Reprobe both candidates for emit, link, and qemu outcome.
- Capture emitted RV64 assembly and any available prepared BIR dumps for the
  function-address store, return, and indirect-call regions.
- Identify whether the first failure is function-address materialization,
  local function-pointer storage, return value transport, indirect-call
  emission, or a separate residual.
- Record the first repair boundary and the focused tests to add.

Completion check:

- `todo.md` names the first repair boundary, assigns each candidate to a
  function-pointer mechanism bucket, and identifies focused runtime tests to
  add.

## Step 2: Add Focused Function Pointer Coverage

Goal: encode function-address storage and indirect-call behavior before
changing RV64 lowering.

Primary targets:

- Backend tests for local function-address store/load.
- Backend tests for indirect calls through loaded and returned function-pointer
  values.

Actions:

- Add a runtime test for storing a function address in a local scalar or
  aggregate home, loading it back, and calling through the pointer.
- Add a runtime test for returning a function pointer and calling through the
  returned value when evidence from `src/00124.c` requires it.
- Ensure tests would fail for placeholder integers, accidental direct-call
  rewrites, missing function-address materialization, or stale local values.
- Keep tests semantic and independent of candidate filenames.

Completion check:

- Focused tests expose the current function-pointer gap and prove runtime use
  through function-pointer values.

## Step 3: Repair Function Address Materialization And Local Storage

Goal: make RV64 prepared lowering represent function symbols as real pointer
values that can be stored to and loaded from local homes.

Primary targets:

- Function-symbol address materialization.
- RV64 prepared local scalar or aggregate storage for pointer values.
- Any prepared value-home or storage plan use needed to preserve function
  pointer values.

Actions:

- Inspect how function-address values are represented before RV64 emission.
- Repair materialization so function symbols can be moved as pointer values
  without becoming placeholder integers or accidental direct-call labels.
- Repair local store/load flow for function-pointer values while preserving
  existing scalar and aggregate-local behavior.
- Re-run focused tests and the `src/00087.c` probe.

Completion check:

- Local function-address store/load focused tests pass, and `src/00087.c`
  either runs under qemu or is classified as a newly exposed non-storage
  residual with emitted-code evidence.

## Step 4: Repair Function Pointer Return And Indirect Call Emission

Goal: make returned or loaded function-pointer values usable as indirect-call
targets on RV64.

Primary targets:

- RV64 function-pointer return value transport.
- Indirect-call target materialization and emission.
- Call-site handling for loaded or returned function-pointer values.

Actions:

- Inspect how returned function-address values reach the caller in `src/00124.c`.
- Repair return-value transport for function pointers when it is the first bad
  mechanism.
- Emit indirect calls through runtime pointer values instead of rewriting them
  into direct calls.
- Re-run focused indirect-call tests and the `src/00124.c` probe.

Completion check:

- Indirect-call focused tests pass, and `src/00124.c` either runs under qemu
  or is classified as a separate residual with concrete emitted-code evidence.

## Step 5: Reprobe And Close Classification

Goal: prove idea 315 acceptance criteria or identify the next separate source
idea.

Primary targets:

- `src/00087.c`
- `src/00124.c`
- All focused function-pointer tests added during this runbook.

Actions:

- Reprobe both candidates for emit, link, and qemu outcome.
- Summarize which function-pointer mechanisms were repaired and which, if any,
  residuals remain outside local/return/indirect-call function-pointer flow.
- Add follow-up source ideas only for separate mechanisms discovered during
  execution.
- Run the supervisor-selected backend guard or broader validation checkpoint.

Completion check:

- Focused backend coverage passes, representative candidates are either qemu
  passes or classified with concrete emitted-code evidence, and no progress
  depends on symbol-name, filename, field-offset, or direct-call substitution
  shortcuts.
