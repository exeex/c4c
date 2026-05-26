# AArch64 c_testsuite 00181 Runtime Regression Reopen Runbook

Status: Active
Source Idea: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md
Activated from: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md

## Purpose

Reopen and repair the current AArch64 runtime regression in
`c_testsuite_aarch64_backend_src_00181_c` after the edge-publication and
absent-selection fallback cleanup batch.

## Goal

Make `c_testsuite_aarch64_backend_src_00181_c` pass again by repairing the
semantic lowering rule that now produces a segmentation fault, then prove the
fix with focused same-feature coverage and matched AArch64 validation.

## Core Rule

Do not special-case `00181`, Tower of Hanoi symbols, or one exact global array
shape. Repair the underlying AArch64 lowering/publication behavior, and do not
weaken c_testsuite expectations.

## Read First

- `ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md`
- `ideas/closed/13_aarch64_cts_00181_recursion_global_array_runtime.md`
- `ideas/closed/17_aarch64_absent_selection_fallback_retirement.md`
- `tests/c/external/c-testsuite/src/00181.c`
- `src/backend/mir/aarch64/codegen/`

## Current Targets

- Primary failing test:
  `c_testsuite_aarch64_backend_src_00181_c`
- Narrow reproduction command:
  `ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00181_c$'`
- Likely investigation surfaces:
  `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`,
  `src/backend/mir/aarch64/codegen/globals.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`,
  and related backend tests under `tests/backend/mir/`.

## Non-Goals

- Do not reopen the full BIR edge value-flow authority migration.
- Do not reopen absent-selection fallback retirement unless evidence directly
  ties the retired path to this regression.
- Do not restore broad target-local fallback selection as a substitute for
  complete prepared facts.
- Do not mark `00181` unsupported, weaken runtime expectations, or accept it
  as a known failure without explicit user approval.
- Do not fold unrelated AArch64 calls, dispatch, or c_testsuite cleanup into
  this route.

## Working Model

Idea 13 previously repaired the `00181` recursion/global-array runtime family,
and its closure recorded `00181` passing with backend AArch64 tests. Idea 17
later closed with matched AArch64 before/after logs where `00181` was the only
remaining failure. Treat this as a reopened runtime regression or baseline
drift family whose cause must be re-established from the current tree.

## Execution Rules

- Start with reproduction and evidence, not implementation.
- Compare current facts against the idea 13 closure assumptions before editing
  code.
- Keep fixes semantic and reusable across same-feature cases.
- Add or update focused backend coverage for the repaired rule before relying
  on the c_testsuite test alone.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- Escalate to broader validation after the focused fix because this route
  touches AArch64 runtime behavior.

## Ordered Steps

### Step 1: Reproduce and classify the current failure

Goal: Establish the current failure mode and gather enough generated evidence
to choose the correct ownership surface.

Actions:

- Build the current tree if needed.
- Run:
  `ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00181_c$'`
- Inspect the failing test source and generated AArch64 output or runner
  artifacts available from the test harness.
- Record whether the first bad behavior appears at global-array addressing,
  recursion/call preservation, call-boundary publication, edge publication, or
  absent-selection fallback retirement.
- Compare observations to the idea 13 closure notes and implementation commits
  named there.

Completion check:

- `todo.md` records the exact reproduction command, observed failure, and the
  most likely failing capability surface with evidence.

### Step 2: Trace the regression against prior repair assumptions

Goal: Explain why the previous `00181` repair no longer holds.

Actions:

- Inspect the prior idea 13 repair area and the cleanup areas from ideas 16/17.
- Trace prepared facts and selected sources through the failing call/global
  array path.
- Identify whether the failure came from edge-publication authority,
  absent-selection fallback retirement, call-boundary publication,
  global-array addressing, recursion/call preservation, or unrelated AArch64
  runtime state.
- Keep the explanation focused on a reusable semantic lowering gap.

Completion check:

- The executor can name the stale assumption, the current missing or incorrect
  fact, and the concrete code surface that should own the fix.

### Step 3: Repair the semantic lowering rule

Goal: Fix the underlying AArch64 lowering/publication behavior without adding
testcase-shaped shortcuts.

Actions:

- Implement the smallest semantic repair in the owning AArch64 backend surface.
- Preserve fail-closed behavior for genuinely missing prepared facts.
- Do not reintroduce broad fallback source reconstruction.
- Keep unrelated call, dispatch, and ABI cleanup out of the patch.

Completion check:

- The narrow `00181` reproduction passes after a fresh build, and code review
  of the diff shows no named-test shortcut or expectation weakening.

### Step 4: Add focused same-feature coverage

Goal: Prove the repaired behavior outside the named c_testsuite case.

Actions:

- Add or update focused backend coverage under `tests/backend/mir/` or a
  comparable existing backend test surface.
- Target the semantic rule identified in Step 2, such as global-array address
  materialization across recursion/calls, call-boundary publication, or edge
  value publication.
- Keep test names and assertions capability-oriented rather than `00181`-only.

Completion check:

- The focused backend test fails before the fix or directly asserts the
  repaired contract, and passes with the implementation change.

### Step 5: Validate and prepare closure evidence

Goal: Prove the fix did not introduce new AArch64 backend or c_testsuite
failures.

Actions:

- Run a fresh build.
- Run the focused backend coverage and `c_testsuite_aarch64_backend_src_00181_c`.
- Run matched broader AArch64 backend/c_testsuite validation chosen by the
  supervisor, typically covering `^backend_aarch64_` plus
  `^c_testsuite_aarch64_backend_` when runtime runner support is available.
- Record whether the regression was caused by the 16/17 batch or by older
  baseline drift that resurfaced during closure validation.

Completion check:

- `test_before.log` and `test_after.log`, when requested by the supervisor,
  are matched in scope; `00181` passes; no new backend or c_testsuite AArch64
  failures are introduced; and the close note has enough cause analysis for
  lifecycle closure.
