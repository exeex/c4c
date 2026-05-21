# AArch64 Recursive Pointer Formal Home Publication Runbook

Status: Active
Source Idea: ideas/open/357_aarch64_recursive_pointer_formal_home_publication.md

## Purpose

Repair same-module recursive call paths where pointer formals assigned to
preserved homes are consumed before those homes are populated on every path.

## Goal

Make recursive pointer-formal home publication semantic enough to advance or
pass `c_testsuite_aarch64_backend_src_00181_c` without weakening proof
contracts or regressing the address-valued call-argument cases repaired by
idea 355.

## Core Rule

Do not fix `00181`, `Hanoi`, `Move`, or one callee-saved register directly.
Every code slice must repair a general pointer-formal home publication
boundary and prove it with focused backend coverage plus representative
external cases.

## Read First

- `ideas/open/357_aarch64_recursive_pointer_formal_home_publication.md`
- `todo.md`
- `test_after.log` from the idea 355 adjacent-case classification
- generated AArch64 artifacts under `build/c_testsuite_aarch64_backend/`
- prepared call plans and selected lowering traces for `00181`
- idea 355 closure notes for already repaired address-valued call-argument
  representatives `00170` and `00189`

## Current Targets

- Primary representative: `c_testsuite_aarch64_backend_src_00181_c`
- Focused backend coverage for pointer formals assigned to callee-saved homes
  and consumed across same-module recursive or helper calls
- Regression representatives from idea 355: `00170` and `00189`

## Non-Goals

- Semantic-BIR dynamic pointer-derived string loads for `00173`
- Frontend or semantic admission failure for `00005`
- External-call symbol or address-valued memory publication already covered by
  ideas 354 and 355
- ABI-wide composite/byval/HFA/f128 work, variadic floating work, dynamic
  stack work, or scalar compare/select residuals
- Expectation, unsupported, runner, timeout, CTest-registration, or proof-log
  changes

## Working Model

Prepared call plans may assign pointer formals to preserved homes such as
callee-saved registers. A recursive or same-module path is only correct if the
incoming pointer formal is published into that home before any later call
argument reload or helper call consumes the home.

The suspected failure is not the existence of a preserved home. The failure is
that some control-flow paths use the home before the formal value has been
published there.

## Execution Rules

- Start by proving the first bad boundary from prepared call/home metadata to
  generated AArch64, not from source names.
- Add or extend focused backend tests before relying on c-testsuite movement.
- Keep each implementation packet limited to pointer-formal home publication
  across same-module recursive or helper-call paths.
- Preserve the already passing idea 355 address-valued call-argument cases in
  every focused proof subset that touches call publication.
- Split through lifecycle if the first bad fact moves into semantic string
  loads, frontend admission, composite ABI, or unrelated scalar/variadic
  lowering.

## Ordered Steps

### Step 1: Localize The Formal-Home Boundary

Goal: identify where pointer formals assigned to preserved homes fail to get
published before recursive or helper-call consumption.

Actions:

- Inspect prepared BIR, call plans, selected lowering state, and generated
  AArch64 for `00181`.
- Identify the incoming pointer formal, assigned home, publication point, and
  later consumer that disagree.
- Confirm whether the first bad fact is path-sensitive home publication rather
  than call-argument selection, symbol materialization, or semantic BIR.

Completion check:

- `todo.md` names the first repair boundary, the smallest representative, the
  relevant formal homes, and rejected adjacent owners.

### Step 2: Add Focused Recursive Pointer-Formal Coverage

Goal: make the failure observable without depending on the c-testsuite source
filename.

Actions:

- Add focused backend coverage for a same-module or recursive call path where
  pointer formals are assigned to preserved homes.
- Ensure the test checks that the preserved homes are populated before any
  recursive/helper-call argument reload consumes them.
- Keep the fixture small enough to distinguish pointer-formal publication from
  address-valued call-argument publication.

Completion check:

- The focused test fails before the repair or directly captures the previously
  missing publication invariant.

### Step 3: Repair Pointer-Formal Home Publication

Goal: populate callee-saved pointer-formal homes on every path that later
reloads or passes those homes.

Actions:

- Repair the AArch64 lowering path that maps incoming pointer formals into
  preserved homes.
- Prefer existing prepared-home, formal-publication, and call-argument helpers
  over new testcase-shaped branches.
- Prove the focused backend coverage and `00181`.

Completion check:

- `00181` advances or passes, and the focused backend test passes without
  expectation, runner, timeout, or filename-specific changes.

### Step 4: Preserve Adjacent Call-Publication Repairs

Goal: ensure the repair does not regress address-valued call-argument behavior
from idea 355.

Actions:

- Run the supervisor-selected focused subset including `00170` and `00189`.
- Include the relevant backend unit/CLI contracts for instruction dispatch,
  memory operands, frame stack calls, and prepared local-argument calls when
  the supervisor selects them.
- Classify any residual that does not share the pointer-formal home boundary.

Completion check:

- `00170` and `00189` remain passing, and `todo.md` records proof scope and
  any unrelated residuals.

### Step 5: Broader Guard And Closure Decision

Goal: prove the owner is stable enough for lifecycle closure or a clean split.

Actions:

- Run the focused proof selected by the supervisor.
- Run a broader backend guard when the supervisor treats the owner as a
  milestone or closure candidate.
- Ask plan-owner to close this idea only when the source idea acceptance
  criteria are satisfied.

Completion check:

- Regression logs and `todo.md` support closure, parking with explicit
  residuals, or a lifecycle split.
