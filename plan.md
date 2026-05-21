# AArch64 String Literal Pointer Null Comparison Runbook

Status: Active
Source Idea: ideas/open/366_aarch64_string_literal_pointer_null_comparison.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused `00112` runtime residual selected by the backend inventory:
direct string-literal pointer/null comparison returns a stale AArch64 register.

## Goal

Make AArch64 lowering materialize and publish a defined boolean result for
string-literal pointer comparisons against null.

## Core Rule

Repair pointer constant comparison/result publication generally. Do not
special-case `00112`, `.str0`, `"abc"`, null, one register, or the emitted
`mov x0, x13; ret` sequence.

## Read First

- `ideas/open/366_aarch64_string_literal_pointer_null_comparison.md`
- `todo.md`
- `test_after.log`
- generated `build/c_testsuite_aarch64_backend/src/00112.c.s`
- semantic/prepared BIR for `tests/c/external/c-testsuite/src/00112.c`
- AArch64 pointer constant, comparison, boolean publication, and return
  lowering paths

## Current Scope

- string-literal pointer constant materialization
- null pointer comparison lowering
- boolean comparison result publication
- scalar return publication for pointer-comparison results
- representative proof for `c_testsuite_aarch64_backend_src_00112_c`

## Non-Goals

- Do not reopen dynamic pointer-derived byte loads from idea 356 without fresh
  evidence.
- Do not repair local pointer reassignment, pointer-derived address scaling,
  indexed aggregate writeback, scalar FP arithmetic, file I/O return counts,
  `sizeof`, aggregate initializer layout, enum bit-field layout, or timeout
  residuals under this plan.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.

## Working Model

The current first bad fact is not a string byte load. Generated `00112` emits
the string literal but returns `x13` directly, so the missing capability is in
the pointer constant comparison/result publication path before scalar return.
Treat stale `x13` as the downstream symptom until localization proves the
first missing boundary.

## Execution Rules

- Start from generated `00112.c.s` and the semantic/prepared records for
  `"abc" == (void *)0`.
- Trace the string-literal pointer constant, null operand, comparison result,
  and return value independently.
- Add focused coverage before or with the repair so the behavior is guarded
  without relying only on `00112`.
- Preserve dynamic pointer-derived string-load behavior and unrelated pointer
  memory owners.
- If `00112` advances after the comparison repair, record the new first bad
  fact in `todo.md` instead of expanding this idea silently.

## Steps

### Step 1: Localize Pointer Comparison Publication Gap

Goal: identify the first backend boundary where the string-literal pointer,
null comparison, boolean result, or return publication is lost.

Primary target: generated `00112.c.s`, semantic/prepared BIR for `00112`, and
AArch64 scalar comparison/result publication helpers.

Actions:

- Inspect semantic and prepared BIR for the string literal pointer constant
  and null comparison.
- Trace whether `.str0` address materialization is absent, comparison lowering
  is skipped, the boolean result is unpublished, or return lowering consumes a
  stale home.
- Name the exact owner boundary and why idea 356 is not the owner.
- Decide the focused coverage shape needed before repair.

Completion check:

- `todo.md` names the first bad fact, owning backend boundary, and coverage
  requirement for string-literal pointer/null comparison publication.

### Step 2: Add Focused Pointer Comparison Coverage

Goal: guard string-literal pointer/null comparison publication independently
of `00112`.

Primary target: backend tests or dump coverage for pointer constant
comparison and scalar boolean return publication.

Actions:

- Add or extend focused coverage for a string-literal pointer compared against
  null.
- Assert the result is materialized as a defined boolean value before return.
- Keep the test semantic, not tied to `.str0`, `x13`, or one emitted text
  sequence.

Completion check:

- Focused coverage fails before the repair or directly guards the missing
  pointer comparison/result publication fact.

### Step 3: Repair General Pointer Constant Comparison

Goal: make AArch64 pointer constant comparisons publish a defined result.

Primary target: the materialization, comparison, result-publication, or return
helper localized in Step 1.

Actions:

- Implement the smallest general repair for string-literal pointer/null
  comparison publication.
- Preserve dynamic pointer-derived string loads and existing pointer memory
  behavior.
- Avoid changes to unrelated pointer/address or aggregate owners unless
  Step 1 proves they share the boundary.
- Run build proof before focused and representative tests.

Completion check:

- Focused coverage passes and generated AArch64 no longer returns stale state
  for the covered pointer comparison shape.

### Step 4: Prove Representative And Classify Residual

Goal: prove `00112` advances past the stale pointer-comparison return and
classify any next first bad fact.

Primary target: focused backend coverage and
`c_testsuite_aarch64_backend_src_00112_c`.

Actions:

- Run the supervisor-delegated build and proof command.
- Inspect generated `00112.c.s` for defined comparison/result publication.
- If `00112` still fails, classify the next first bad fact and decide whether
  it remains in this idea or needs lifecycle handoff.
- Ask the supervisor whether broader backend-regex or regression-guard proof
  is needed before closure.

Completion check:

- `00112` no longer fails because `main` returns stale `x13`, and any
  remaining failure is explicitly classified in `todo.md`.
