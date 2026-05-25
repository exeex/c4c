# AArch64 c_testsuite 00181 Recursion Global Array Runtime Runbook

Status: Active
Source Idea: ideas/open/13_aarch64_cts_00181_recursion_global_array_runtime.md

## Purpose

Repair the AArch64 backend runtime failure represented by
`c_testsuite_aarch64_backend_src_00181_c` after the prior-preservation
source-selection family was split out.

## Goal

Identify and fix the semantic lowering rule behind the `00181` recursion and
global-array runtime mismatch.

## Core Rule

Fix the underlying recursion, global array, pointer/index arithmetic, or
call/return lowering behavior. Do not special-case `00181`, Tower of Hanoi
symbols, exact global layouts, or c_testsuite expectations.

## Read First

- `ideas/open/13_aarch64_cts_00181_recursion_global_array_runtime.md`
- `tests/c/external/c-testsuite/src/00181.c`
- AArch64 backend lowering for globals, addresses, calls, returns, and indexed
  memory access as indicated by the reproduction.
- Existing focused backend tests near the first identified lowering owner.

## Current Targets

- `c_testsuite_aarch64_backend_src_00181_c`
- The first semantic owner proven by reproduction and tracing.
- Focused backend coverage for the repaired rule.

## Non-Goals

- Do not reopen the prior-preservation selected fallback.
- Do not change unrelated c_testsuite expectations or unsupported markings.
- Do not fold `00216` or `00204` into this route without new evidence.
- Do not perform broad AArch64 calls or ABI cleanup unless tracing proves it is
  the direct `00181` cause.

## Working Model

`00181` remained red after the call-boundary prior-preservation family was
repaired. Treat it as a separate runtime correctness problem until evidence
shows otherwise. The likely suspects are recursion, global array addressing,
pointer/index arithmetic, and call/return value handling.

## Execution Rules

- Start from a fresh reproduction of `c_testsuite_aarch64_backend_src_00181_c`.
- Trace from observed runtime behavior to the first incorrect IR, MIR, machine
  instruction, or runtime value.
- Prefer a focused unit/backend test that proves the semantic rule independent
  of the named c_testsuite file.
- Keep proof in `todo.md`; do not edit the source idea for routine execution
  notes.
- Escalate to reviewer or plan-owner only if evidence shows this is actually a
  different initiative than `00181` recursion/global-array runtime repair.

## Steps

### Step 1: Reproduce And Localize Runtime Mismatch

- Run the narrow `c_testsuite_aarch64_backend_src_00181_c` proof selected by
  the supervisor.
- Inspect the failing program behavior and identify the first bad observable:
  wrong global array contents, wrong index/address, wrong recursive call state,
  wrong return value, or wrong memory access.
- Use dumps or targeted instrumentation only enough to locate the earliest bad
  lowering owner.

Completion check: the failure is reproducible, and the suspected semantic
owner is narrowed to a concrete frontend, HIR/LIR, BIR, or AArch64 backend
path.

### Step 2: Prove The Missing Semantic Rule

- Build a focused reduced case or backend test around the same feature shape
  without naming `00181` as the implementation condition.
- Compare nearby same-feature behavior so the fix cannot be only a named-case
  shortcut.
- Document in `todo.md` why the issue is recursion, global-array addressing,
  pointer/index arithmetic, or call/return handling.

Completion check: a focused failing test or trace demonstrates the missing
semantic lowering rule and excludes the old prior-preservation fallback route.

### Step 3: Repair The Narrow Owner

- Update only the first proven semantic owner.
- Preserve existing behavior outside the failing feature shape.
- Add or update focused coverage for the repaired rule.
- Avoid broad rewrites of call planning, aggregate layout, or ABI logic unless
  Step 1 and Step 2 prove that owner is directly responsible.

Completion check: the focused test passes, and the implementation does not
contain `00181`-specific or symbol-name-specific matching.

### Step 4: Validate The 00181 Family

- Run the delegated narrow proof for `c_testsuite_aarch64_backend_src_00181_c`.
- Run the focused backend/unit coverage added or updated in Step 3.
- Run any nearby same-feature tests needed to reject a named-test-only fix.
- Record exact proof commands and results in `todo.md`.

Completion check: `00181` and focused same-feature proof pass without weakened
contracts or unrelated expectation changes.
