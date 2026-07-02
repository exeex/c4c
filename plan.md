# BIR Runtime Intrinsic Memory Producer Admission Runbook

Status: Active
Source Idea: ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md

## Purpose

Repair BIR runtime/intrinsic memory producer admission for the current exact
`semantic lir_to_bir` rows classified as `memcpy` and `memset` memory-effect
failures.

## Goal

Publish the BIR facts required for intrinsic memory effects so representative
RV64 rows advance because the semantic producer records memory behavior, not
because downstream consumers infer, bypass, or special-case it.

## Core Rule

Treat `memcpy` and `memset` as producer-admission work. Do not replace them
with target runtime substitutions, named testcase lowering, expectation
rewrites, unsupported-marker downgrades, or weaker semantic checks.

## Read First

- `ideas/open/559_bir_runtime_intrinsic_memory_producer_admission.md`
- Runtime/intrinsic memory handling in `src/backend/bir/lir_to_bir/`
- Existing focused BIR note tests in `tests/backend/bir/`
- Representative RV64 rows:
  - `tests/c/external/gcc_torture/src/20000703-1.c`
  - `tests/c/external/gcc_torture/src/20041218-1.c`

## Scope

- Identify the BIR producer path responsible for `memcpy` and `memset`
  memory-effect facts.
- Add focused BIR tests for both intrinsic families.
- Repair semantic admission for both families.
- Prove with focused backend tests and RV64 representative rows.

## Non-Goals

- Do not implement generic load, GEP, store, scalar/local-memory, or alloca
  work unless inspection proves a shared helper must change.
- Do not implement runtime library substitution or target-specific call
  replacement as the main repair.
- Do not change expected outputs, unsupported markers, or allowlists to claim
  progress.
- Do not fold this lane into generic local-memory work without concrete row
  and code evidence of a shared producer boundary.

## Working Model

`memcpy` and `memset` rows are memory-behavior facts that must be published by
the BIR producer before prepared/RV64 consumers can safely lower them. The
initial route should inspect current representative rows, isolate the first
missing semantic fact, and then repair the shared producer boundary only as far
as the two intrinsic families justify.

## Execution Rules

- Keep each code slice focused on a producer boundary and its direct tests.
- Prefer focused BIR coverage before or with producer repairs.
- Preserve both intrinsic families in the proof plan; proving only one family
  is not enough to close the idea.
- If inspection proves `memcpy` and `memset` do not share a boundary, stop and
  ask the plan owner to split the source idea instead of forcing one repair.
- Use `test_after.log` for executor backend proof unless the supervisor
  delegates another canonical artifact.
- Escalate to nearby generic local-memory coverage only when shared helpers are
  changed.

## Steps

### Step 1: Inspect Intrinsic Memory Admission Boundary

Goal: Locate the first missing producer fact for representative `memcpy` and
`memset` rows.

Primary targets:

- `tests/c/external/gcc_torture/src/20000703-1.c`
- `tests/c/external/gcc_torture/src/20041218-1.c`

Actions:

- Reproduce both representative rows with the RV64 backend harness.
- Capture the current semantic admission diagnostics and the BIR/LLVM shapes
  that reach the producer.
- Identify the BIR producer path that should publish intrinsic memory effects.
- Decide whether `memcpy` and `memset` share one repair boundary or require a
  lifecycle split.

Completion check:

- `todo.md` records the exact missing producer fact, owning files, diagnostic
  evidence, representative log paths, and whether the next step can repair a
  shared boundary.

### Step 2: Add Focused Intrinsic Memory Producer Coverage

Goal: Lock down the desired BIR fact publication for both intrinsic families.

Actions:

- Add focused BIR note tests for `memcpy` memory-effect fact publication.
- Add focused BIR note tests for `memset` memory-effect fact publication.
- Keep the tests semantic and helper-oriented; do not encode gcc_torture
  filename shortcuts.

Completion check:

- Focused backend tests fail before the repair or are added with the repair in
  the same slice, and the assertions describe both intrinsic families.

### Step 3: Repair Shared Intrinsic Memory Producer Admission

Goal: Publish the missing memory-effect facts for the shared intrinsic
producer boundary.

Actions:

- Repair the BIR producer path identified in Step 1.
- Keep changes inside the intrinsic-memory producer boundary unless a shared
  local-memory helper is proven necessary.
- If a shared local-memory helper changes, include nearby generic coverage so
  the change is not intrinsic-only overfit.

Completion check:

- Backend BIR tests pass.
- Representative RV64 rows no longer fail in the same runtime/intrinsic
  semantic producer admission mode.
- Any remaining failures are classified as downstream ownership with log
  evidence.

### Step 4: Reconcile Runtime Intrinsic Memory Representatives

Goal: Confirm both intrinsic families advanced and decide whether the source
idea is complete or needs a split.

Actions:

- Re-run a narrow RV64 subset covering current `memcpy` and `memset`
  representatives.
- Compare representative outcomes against the Step 1 diagnostics.
- Record whether any residual rows are generic local-memory, prepared/RV64,
  F128, or evidence-gap work.

Completion check:

- `todo.md` records the final representative outcomes and recommends close,
  split, or further runbook work.
