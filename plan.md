# AArch64 Backend Result Register Runtime Nonzero Plan

Status: Active
Source Idea: ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md
Supersedes Active Runbook: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md

## Purpose

Repair the AArch64 backend result-register/codegen failure exposed by the
runtime c-testsuite route after idea 276 made that route execute truthfully.

## Goal

Make integer return expressions materialize their result in the AArch64 ABI
return register before `ret`, so `src/00003.c` and nearby same-feature cases
pass the backend assembly runtime route without expectation changes.

## Core Rule

Fix the semantic backend/codegen rule. Do not use filename-specific matching,
c-testsuite shortcuts, LLVM IR fallback, or expectation weakening as progress.

## Read First

- `ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md`
- `todo.md`
- `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`
- `test_after.log`, if still present, for the latest focused route evidence
- AArch64 backend lowering, register-allocation, and assembly emission code
  touched by integer return-value codegen

## Current Targets

- AArch64 backend return-value lowering/result-register placement.
- Runtime proof for `tests/c/external/c-testsuite/src/00003.c`.
- Regression proof for already passing route smoke cases `00001.c` and
  `00002.c`.
- At least one nearby arithmetic-return case that proves the repair is not
  named-case overfit.

## Non-Goals

- Do not change c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, or CTest expectations.
- Do not edit the runtime runner or native-host detection unless the supervisor
  opens a separate route issue.
- Do not claim progress from LLVM IR fallback.
- Do not perform broad unrelated AArch64 ABI rewrites.
- Do not fold additional backend failure families into this plan; split new
  owner layers into focused follow-up ideas.

## Working Model

Idea 276 proved that the AArch64 backend c-testsuite runtime route can run
simple cases on this host. `00001.c` and `00002.c` pass through backend `.s`,
clang assembler input, executable runtime, and empty expected-output compare.
`00003.c` now fails at runtime with nonzero exit because generated assembly
computes `w0 - 4` into `w19` and returns without writing the computed value
back to `w0`.

The likely owner layer is AArch64 backend return-value codegen or register
placement around return expressions. The executor should verify the exact
compiler path before changing code.

## Execution Rules

- Keep each packet bounded to investigation, one semantic repair, or one proof
  update.
- Update `todo.md` with the current packet result and exact proof command.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets unless the supervisor delegates another artifact.
- Prefer AST-backed symbol queries via `c4c-clang-tools` for large C++ backend
  files.
- Treat a change as route drift if its main effect is making only `00003.c`
  pass without repairing return-value result-register placement.

## Steps

### Step 1: Localize Return Result Register Ownership

Goal: identify the backend path that lowers C integer return expressions and
chooses the final AArch64 result register.

Actions:

- Inspect the failing `00003.c` assembly and, if useful, dumps for the
  corresponding HIR/BIR/backend route.
- Locate the return-expression lowering, AArch64 instruction selection, and
  any move/register-allocation logic responsible for values returned from a
  function.
- Compare a passing trivial return case with `00003.c` to determine why the
  computed subtraction remains in `w19`.
- Record the smallest semantic repair point and a focused proof command in
  `todo.md`.

Completion check:

- `todo.md` names the owner function or backend stage, explains why the result
  misses `w0`, and gives the exact proof subset for the repair packet.

### Step 2: Repair Return-Value Materialization

Goal: ensure integer return expression values are in the ABI result register
before function return.

Actions:

- Implement the smallest semantic backend/codegen change at the owner point
  identified in Step 1.
- Preserve existing behavior for direct literal/argument returns and ordinary
  temporary register use.
- Avoid case-specific checks for c-testsuite filenames, expression text, or
  exact instruction sequences.
- Build the affected target before runtime proof.

Completion check:

- The project builds for the affected backend target, and generated assembly
  for the failing case writes or moves the computed return value into `w0`
  before `ret`.

### Step 3: Prove Focused Runtime And Nearby Coverage

Goal: prove the repair through the same backend runtime route and reject
testcase overfit.

Actions:

- Run the focused AArch64 backend c-testsuite route for `00001.c`, `00002.c`,
  and `00003.c`.
- Add or run at least one nearby arithmetic-return backend/runtime case that
  exercises the same result-register rule without being `00003.c` shaped.
- Confirm no LLVM IR fallback was used and no expectations were weakened.
- Preserve exact proof output in `test_after.log`.

Completion check:

- `00003.c`, the already passing smoke cases, and the nearby same-feature proof
  pass with backend `.s -> clang -x assembler -> executable -> runtime ->
  expected-output` evidence.

### Step 4: Review Residual Scope

Goal: decide whether this focused idea is complete or whether additional
failure families should be split.

Actions:

- Compare the implemented behavior and proof against
  `ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md`.
- Confirm no unrelated runtime-route or broad ABI work was folded into this
  plan.
- If complete, request plan-owner closure review.
- If proof exposes a distinct backend/codegen/BIR issue, create or request a
  focused follow-up idea instead of expanding this plan.

Completion check:

- `todo.md` clearly says whether idea 277 is ready for closure review or names
  the exact blocker/follow-up owner layer.
