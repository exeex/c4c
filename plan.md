# AArch64 Return Zero Runtime Promotion Runbook

Status: Active
Source Idea: ideas/open/19_backend_aarch64_return_zero_runtime_promotion_plan.md
Activated from: ideas/open/__backend_port_plan.md

## Purpose

Close the remaining runtime-harness gap for the minimal AArch64 constant-return slice without expanding scope beyond `return_zero`.

## Goal

Promote `tests/c/internal/backend_case/return_zero.c` to asm-backed execution using the existing bounded constant-return emitter path.

## Core Rule

Keep this slice mechanical: inspect the runtime LIR first, reuse the existing constant-return matcher if it already applies, and stop if the case requires new lowering work.

## Read First

- `ideas/open/19_backend_aarch64_return_zero_runtime_promotion_plan.md`
- `tests/c/internal/backend_case/return_zero.c`
- `tests/c/internal/InternalTests.cmake`
- `src/backend/aarch64/codegen/emit.cpp`

## Scope

- runtime promotion of `return_zero` to `BACKEND_OUTPUT_KIND=asm`
- bounded validation that the existing AArch64 constant-return emitter path still matches the runtime shape
- targeted backend/runtime checks plus full-suite monotonic regression validation

## Non-Goals

- new lowering families in `src/backend/aarch64/codegen/emit.cpp`
- synthetic backend fixture expansion unless the runtime shape unexpectedly diverges
- unrelated backend runtime promotions

## Working Model

`return_zero.c` should already lower to the same minimal single-function constant-return slice that the AArch64 emitter knows how to materialize directly in asm.

## Execution Rules

- inspect the runtime-emitted LIR before changing `tests/c/internal/InternalTests.cmake`
- only touch `src/backend/aarch64/codegen/emit.cpp` if the runtime shape exposes a bounded matcher gap
- validate with targeted tests first, then run the full regression workflow and compare before/after results

## Ordered Steps

### Step 1: Confirm the bounded runtime shape

Goal: prove that `return_zero` matches the existing constant-return asm seam.

Primary targets:

- `tests/c/internal/backend_case/return_zero.c`
- `src/backend/aarch64/codegen/emit.cpp`

Actions:

- inspect the runtime-emitted LIR for `return_zero.c`
- compare it against the current minimal constant-return matcher/emitter path
- stop and split a new idea if the runtime shape needs new lowering work

Completion check:

- the runtime `return_zero` shape is confirmed to be compatible with the existing bounded constant-return asm path

### Step 2: Promote the runtime case

Goal: switch `return_zero` to asm-backed execution with the smallest possible test-harness change.

Primary target:

- `tests/c/internal/InternalTests.cmake`

Actions:

- set `return_zero` to `BACKEND_OUTPUT_KIND=asm`
- keep the expected exit code unchanged
- avoid widening the change to unrelated backend cases

Completion check:

- `backend_runtime_return_zero` is configured to execute through the AArch64 asm toolchain path

### Step 3: Validate and regressions-check

Goal: prove the runtime promotion is correct and non-regressive.

Primary targets:

- `tests/c/internal/backend_case/return_zero.c`
- `test_before.log`
- `test_after.log`

Actions:

- run targeted checks for `backend_runtime_return_zero` and nearby constant-return backend coverage
- run the full regression workflow and compare before/after results
- record any blockers or unexpected runtime-shape differences in `plan_todo.md`

Completion check:

- `backend_runtime_return_zero` passes on the asm path
- full-suite results are monotonic with zero newly failing tests
