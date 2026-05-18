# AArch64 Backend Local Operand Materialization Runtime Nonzero Plan

Status: Active
Source Idea: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md
Supersedes Active Runbook: ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md

## Purpose

Repair the AArch64 backend operand/local-value materialization failure exposed
after the result-register owner layer from idea 277 was fixed.

## Goal

Make local integer values used as arithmetic operands materialize from their
actual source value or storage before the returned expression is computed, so
`src/00003.c` passes the AArch64 backend runtime route without expectation
changes.

## Core Rule

Fix the semantic backend/codegen rule for local operand materialization. Do not
use filename-specific matching, c-testsuite shortcuts, LLVM IR fallback,
expectation weakening, or a regression of the result-register repair as
progress.

## Read First

- `ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md`
- `todo.md`
- `ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md`
- `test_after.log`, if still present, for the latest focused route evidence
- AArch64 backend lowering, storage-plan, assignment, load/materialization, and
  assembly emission code touched by local integer operands

## Current Targets

- AArch64 backend lowering for local integer values used as operands.
- Runtime proof for `tests/c/external/c-testsuite/src/00003.c`.
- Regression proof for already passing route smoke cases `00001.c` and
  `00002.c`.
- Focused proof that the idea 277 result-register fix remains intact.
- At least one nearby local-value arithmetic case that proves the repair is not
  named-case overfit.

## Non-Goals

- Do not rework return-result register placement except to preserve the idea
  277 repair.
- Do not change c-testsuite `.expected` sidecars, allowlists, unsupported
  classifications, or CTest expectations.
- Do not edit the runtime runner or native-host detection.
- Do not claim progress from LLVM IR fallback.
- Do not perform broad unrelated AArch64 ABI or register-allocation rewrites.
- Do not fold additional backend failure families into this plan; split new
  owner layers into focused follow-up ideas.

## Working Model

Idea 277 fixed the original result-register failure: `00003.c` no longer emits
`sub w19, w0, #4; ret` and now returns through `w0`. The remaining assembly
shape `sub w0, w0, #4; ret` still fails because the left operand is incoming
`w0`, not the source-local `x = 4` value. The likely owner layer is local
assignment/reference materialization or storage-plan lowering for operands
feeding arithmetic expressions.

The executor should verify the exact compiler path before changing code.

## Execution Rules

- Keep each packet bounded to investigation, one semantic repair, or one proof
  update.
- Update `todo.md` with the current packet result and exact proof command.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets unless the supervisor delegates another artifact.
- Prefer AST-backed symbol queries via `c4c-clang-tools` for large C++ backend
  files.
- Treat a change as route drift if its main effect is making only `00003.c`
  pass without repairing local operand materialization.

## Steps

### Step 1: Localize Local Operand Ownership

Goal: identify the backend path that lowers local integer assignments and
references into operands for AArch64 arithmetic expressions.

Actions:

- Inspect the failing `00003.c` assembly and, if useful, dumps for the
  corresponding HIR/BIR/backend route.
- Locate the lowering path for the local `x = 4` value and its later use as
  the left subtraction operand.
- Compare a passing trivial return case with the failing local arithmetic case
  to determine why the operand remains tied to incoming `w0`.
- Record the smallest semantic repair point and a focused proof command in
  `todo.md`.

Completion check:

- `todo.md` names the owner function or backend stage, explains why the local
  operand reads incoming `w0`, and gives the exact proof subset for the repair
  packet.

### Step 2: Repair Local Operand Materialization

Goal: ensure local integer values used as expression operands are materialized
from their actual value or storage before AArch64 arithmetic emission.

Actions:

- Implement the smallest semantic backend/codegen change at the owner point
  identified in Step 1.
- Preserve direct literal returns, direct argument returns, ordinary temporary
  register use, and the idea 277 return-result register behavior.
- Avoid case-specific checks for c-testsuite filenames, expression text, or
  exact instruction sequences.
- Build the affected target before runtime proof.

Completion check:

- The project builds for the affected backend target, and generated assembly
  for the motivating case no longer uses unrelated incoming `w0` as the local
  value operand.

### Step 3: Prove Runtime And Nearby Coverage

Goal: prove the repair through the same backend runtime route and reject
testcase overfit.

Actions:

- Run the focused AArch64 backend c-testsuite route for `00001.c`, `00002.c`,
  and `00003.c`.
- Run the focused backend return-lowering coverage from idea 277 to prove the
  result-register repair remains intact.
- Add or run at least one nearby local-value arithmetic backend/runtime case
  that exercises the same operand materialization rule without being
  `00003.c` shaped.
- Confirm no LLVM IR fallback was used and no expectations were weakened.
- Preserve exact proof output in `test_after.log`.

Completion check:

- `00003.c`, the already passing smoke cases, the result-register regression
  coverage, and the nearby same-feature proof pass with backend `.s -> clang
  -x assembler -> executable -> runtime -> expected-output` evidence where
  applicable.

### Step 4: Review Residual Scope

Goal: decide whether this focused idea is complete or whether additional
failure families should be split.

Actions:

- Compare the implemented behavior and proof against
  `ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md`.
- Confirm no unrelated runtime-route, broad ABI, or register-allocation work
  was folded into this plan.
- If complete, request plan-owner closure review.
- If proof exposes a distinct backend/codegen/BIR issue, create or request a
  focused follow-up idea instead of expanding this plan.

Completion check:

- `todo.md` clearly says whether idea 278 is ready for closure review or names
  the exact blocker/follow-up owner layer.
