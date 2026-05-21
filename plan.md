# AArch64 Sizeof Loop-Bound Stack Home Publication Runbook

Status: Active
Source Idea: ideas/open/376_aarch64_sizeof_loop_bound_stack_home_publication.md
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the scalar constant or `sizeof` publication gap where AArch64 loop
compares read compile-time loop bounds from stack homes that were never
initialized.

## Goal

Make compile-time scalar loop bounds available to AArch64 loop compare
consumers without relying on unpublished stack slots.

## Core Rule

Fix the semantic publication path for constant loop-bound values; do not
special-case `00205`, one `sizeof` expression, one frame offset, or one
emitted compare sequence.

## Read First

- `ideas/open/376_aarch64_sizeof_loop_bound_stack_home_publication.md`
- `todo.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `build/c_testsuite_aarch64_backend/src/00205.c.s`
- source for `c_testsuite` case `00205.c`
- current `test_after.log`

## Current Scope

- AArch64 scalar constants and `sizeof`/constant-binary values used as loop
  bounds.
- Prepared value storage and selected loop-compare operand publication for
  compile-time constants.
- Representative external case `c_testsuite_aarch64_backend_src_00205_c`.

## Non-Goals

- Do not repair global aggregate data emission for `cases`; current evidence
  shows the data is emitted.
- Do not reopen closed selected-value, aggregate-address, or aggregate-layout
  owners without fresh first-bad-fact evidence.
- Do not include external/libc call-result publication, scalar FP
  materialization, complex aggregate initializer/relocation, dynamic-stack
  timeout, or shift/type-promotion timeout work.
- Do not change expectations, unsupported classifications, runners, timeout
  policy, proof-log policy, or CTest registration.

## Working Model

`00205` fails before printing any rows. The outer and inner loop bounds are
compile-time expressions, but generated AArch64 compares loop indices against
values loaded from unpublished stack slots. The repair should either publish
the compile-time value into the selected home before the compare or select a
representation that materializes the constant directly for the compare.

## Execution Rules

- Start by localizing the first bad fact across source, prepared/BIR/MIR, and
  generated AArch64 artifacts.
- Prefer small backend coverage that proves constant or `sizeof` loop bounds
  cannot read uninitialized homes.
- Keep the fix at the value-publication or selected-operand layer indicated by
  localization; avoid broad aggregate or loop-lowering rewrites unless the
  evidence requires them.
- Treat any new `00205` failure after this first bad fact is removed as a
  reclassification point, not automatic scope expansion.

## Steps

### Step 1: Localize Constant Loop-Bound Publication

Goal: identify where the `sizeof`/constant-binary loop-bound value loses its
materialized value or receives an unpublished stack home.

Primary target: `00205.c` source, prepared dumps/helpers, and
`build/c_testsuite_aarch64_backend/src/00205.c.s`.

Actions:

- Trace the outer and inner loop-bound expressions from source through the
  prepared value records and selected compare operands.
- Identify whether the selected compare expects a stack home, register, or
  immediate for each bound.
- Confirm the stores, if any, that should publish the bound value before the
  first compare.
- Record the smallest backend subsystem boundary that owns the missing
  publication.

Completion check:

- `todo.md` records the concrete first bad fact and the repair boundary before
  implementation begins.

### Step 2: Repair Scalar Constant Home Publication

Goal: make compile-time scalar loop bounds available to selected AArch64 loop
  compare consumers.

Primary target: the localized value-publication, prepared-value, or selected
operand lowering path from Step 1.

Actions:

- Implement the narrow repair at the localized publication boundary.
- Ensure constants are published to required homes before first use or are
  selected as direct materialized operands where that is the existing model.
- Keep stack-home behavior consistent for nearby scalar constant consumers.
- Avoid adding filename, expression-shape, offset, or instruction-text
  shortcuts.

Completion check:

- Focused backend coverage and the `00205` representative advance past the
  unpublished loop-bound stack-home failure.

### Step 3: Add Focused Coverage

Goal: guard the general constant or `sizeof` loop-bound publication behavior.

Primary target: local backend tests or focused c-testsuite proof selected by
the supervisor.

Actions:

- Add or update narrow backend coverage for compile-time loop bounds consumed
  by generated compares.
- Include a shape that would fail if the compare reads an uninitialized stack
  home.
- Keep coverage semantic; do not assert one emitted offset or exact
  instruction sequence unless existing backend test style requires it.

Completion check:

- The focused proof scope passes and would catch the old unpublished
  stack-home behavior.

### Step 4: Validate And Reclassify

Goal: prove the repair and decide whether this focused owner is complete.

Primary target: supervisor-selected focused tests plus any backend-regex
subset needed for regression confidence.

Actions:

- Run the delegated build and proof command exactly.
- Check whether `c_testsuite_aarch64_backend_src_00205_c` passes or exposes a
  new first bad fact.
- Preserve any new residual classification in `todo.md` without broadening
  this idea silently.
- Hand back to the supervisor for broader validation and lifecycle close.

Completion check:

- Proof results are recorded in `todo.md`, and any remaining `00205` residual
  is classified outside the old loop-bound stack-home publication failure.
