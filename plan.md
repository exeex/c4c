# RV64 F64 Global-Memory Consumption Runbook

Status: Active
Source Idea: ideas/open/563_rv64_f64_global_memory_consumption.md

## Purpose

Teach the RV64 object route to consume prepared non-F128 floating-point global
memory accesses where prepared facts already describe the target object and
access.

## Goal

Move `src/20001121-1.c` off the current RV64 scalar global-memory type-gate
diagnostic for prepared `double` / `F64` global loads, without fabricating
missing prepared facts in target lowering.

## Core Rule

Consume only prepared global-memory facts that already exist. Do not infer
symbols, object data, relocation authority, or initializer shape inside RV64,
and do not change diagnostics, unsupported markers, allowlists, expected
outputs, or pass/fail accounting as progress.

## Read First

- `ideas/open/563_rv64_f64_global_memory_consumption.md`
- `build/agent_state/548_step2_global_data_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20001121-1.c/case.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- Existing backend tests for RV64 prepared global-memory object emission

## Scope

- RV64 object-route consumption of prepared `double` / `F64` global-memory
  accesses.
- The current representative `src/20001121-1.c`.
- Focused backend or target tests that prove the prepared fact path, not a
  filename-specific lowering path.
- Residual-owner classification if the representative advances to a later
  failure.

## Non-Goals

- Do not repair prepared object-data production or zero-fill contracts here.
- Do not implement F128, long-double, external soft-float, or quarantine policy
  work.
- Do not generalize global initializer production or stack-frame lowering.
- Do not touch FPR callee-saved slot support or unrelated scalar/FPR residuals.
- Do not special-case `src/20001121-1.c`, exact diagnostic text, or pass/fail
  accounting.

## Working Model

Prepared global-memory facts are coherent enough for RV64 to classify the
representative access, but the target object route rejects the access through
an integer/pointer-oriented type gate. The expected repair is in RV64
consumption: extend the prepared global-memory object route to handle
non-F128 `F64` access while preserving fail-closed behavior when prepared facts
are absent.

## Execution Rules

- Start by reconfirming the current representative diagnostic and the exact
  target gate that rejects the prepared `F64` access.
- Add focused coverage before or with the repair so the semantic target type
  and prepared fact requirements are checked directly.
- Keep prepared producer changes out of scope unless inspection proves the
  recorded evidence is stale and prepared facts are no longer coherent.
- Preserve existing integer/pointer global-memory behavior and prepared-fact
  absence rejection paths.
- Record proof commands, log paths, and any downstream residual owner in
  `todo.md`.

## Steps

### Step 1: Inspect F64 Global-Memory Boundary

Goal: Reconfirm the current first bad fact for `src/20001121-1.c` and locate
the RV64 object-route gate that rejects prepared `F64` global memory.

Primary target:

- `src/20001121-1.c`

Actions:

- Re-run or inspect the representative backend log and capture the current
  `unsupported_global_data` evidence.
- Trace the prepared global-memory fact path into RV64 object emission.
- Confirm whether prepared object facts, relocation facts, and target type facts
  are already present before the RV64 gate.
- Record whether the boundary is still RV64 consumer-owned or has moved back to
  a prepared producer.

Completion check:

- `todo.md` records current diagnostics, log paths, owning code boundary, and a
  concrete next packet for focused coverage or repair.

### Step 2: Add Focused F64 Global-Memory Coverage

Goal: Add tests that prove RV64 can consume a prepared non-F128 `F64`
global-memory access through the object route.

Actions:

- Add focused backend or target coverage for a prepared `double` / `F64` global
  load using prepared facts.
- Assert the prepared fact requirement remains explicit; absence of prepared
  object or access facts must still fail closed.
- Keep existing integer and pointer global-memory coverage stable.

Completion check:

- Focused coverage fails before the repair or directly proves the repaired
  contract, backend proof is recorded in `todo.md`, and no expectations or
  unsupported markers are weakened.

### Step 3: Repair RV64 F64 Global-Memory Consumption

Goal: Extend the RV64 object route to lower prepared `F64` global-memory
accesses while preserving prepared/RV64 ownership boundaries.

Actions:

- Implement the minimal RV64 target-consumer repair at the verified owner.
- Reuse existing prepared global-memory facts and target type information.
- Do not synthesize missing prepared facts in RV64 lowering.
- Preserve fail-closed behavior for unsupported floating widths, F128,
  long-double, malformed prepared facts, and unrelated global-data shapes.
- Run focused tests and the backend subset selected by the supervisor.

Completion check:

- Backend tests pass, focused `F64` global-memory coverage passes, and
  `todo.md` records the files changed plus proof commands.

### Step 4: Reconcile Representative And Residual Owner

Goal: Prove whether `src/20001121-1.c` moved off the scalar global-memory
type-gate diagnostic and classify any later failure.

Actions:

- Re-run the representative allowlist row with verbose failure logs.
- Confirm the old RV64 scalar global-memory type-gate diagnostic no longer
  appears.
- If the row advances to a downstream owner, record the exact owner and keep
  distinct work in a separate source idea.
- Recommend close, split, or continue for this source idea.

Completion check:

- `todo.md` records representative outcome, log path, residual owner if any,
  and the lifecycle recommendation for this active idea.
