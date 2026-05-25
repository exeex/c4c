# C Aggregate Initializer Compound Literal Layout Runbook

Status: Active
Source Idea: ideas/open/14_c_aggregate_initializer_compound_literal_layout.md

## Purpose

Repair the aggregate initializer and compound literal layout failure
represented by `c_testsuite_aarch64_backend_src_00216_c` after the
prior-preservation source-selection family was split out.

## Goal

Identify and fix the semantic layout or initialization rule behind the `00216`
runtime mismatch.

## Core Rule

Fix the underlying aggregate initializer, compound literal, object layout, or
copy/materialization behavior. Do not special-case `00216`, one exact
initializer spelling, or c_testsuite expectations.

## Read First

- `ideas/open/14_c_aggregate_initializer_compound_literal_layout.md`
- `tests/c/external/c-testsuite/src/00216.c`
- Frontend aggregate initializer and compound literal lowering.
- HIR/LIR, BIR, and AArch64 backend aggregate object layout or materialization
  paths indicated by reproduction.
- Existing focused aggregate initializer, compound literal, and aggregate layout
  tests near the first identified owner.

## Current Targets

- `c_testsuite_aarch64_backend_src_00216_c`
- The first semantic owner proven by reproduction and tracing.
- Focused coverage for the repaired initializer, compound literal, or aggregate
  layout rule.

## Non-Goals

- Do not reopen the prior-preservation selected fallback.
- Do not perform ABI-wide aggregate calling convention rewrites unless tracing
  proves that ABI lowering is the direct `00216` cause.
- Do not weaken c_testsuite expectations or unsupported markings.
- Do not fold `00181` or `00204` into this route without new evidence.

## Working Model

`00216` remained red after the call-boundary prior-preservation family was
repaired and after `00181` was split and closed. Treat it as a separate
aggregate initializer and compound literal layout problem until evidence shows
otherwise. The likely suspects are initializer lowering, compound literal
storage, aggregate field layout, object copying, and target materialization of
aggregate values.

## Execution Rules

- Start from a fresh reproduction of
  `c_testsuite_aarch64_backend_src_00216_c`.
- Trace from observed runtime behavior to the first incorrect frontend, HIR/LIR,
  BIR, or AArch64 backend fact.
- Prefer focused coverage that proves the semantic aggregate layout rule
  independent of the named c_testsuite file.
- Keep proof in `todo.md`; do not edit the source idea for routine execution
  notes.
- Escalate to reviewer or plan-owner only if evidence shows this is actually a
  different initiative than aggregate initializer and compound literal layout
  repair.

## Steps

### Step 1: Reproduce And Localize Aggregate Layout Failure

- Run the narrow `c_testsuite_aarch64_backend_src_00216_c` proof selected by
  the supervisor.
- Inspect the failing program behavior and identify the first bad observable:
  wrong initialized field value, wrong compound literal storage, wrong aggregate
  copy, wrong field offset, or wrong materialized address/value.
- Use dumps or targeted instrumentation only enough to locate the earliest bad
  lowering owner.

Completion check: the failure is reproducible, and the suspected semantic owner
is narrowed to a concrete frontend, HIR/LIR, BIR, or AArch64 backend path.

### Step 2: Prove The Missing Layout Rule

- Build a focused reduced case or backend test around the same initializer or
  compound literal shape without naming `00216` as the implementation
  condition.
- Compare nearby aggregate initialization behavior so the fix cannot be only a
  named-case shortcut.
- Document in `todo.md` why the issue is aggregate layout, compound literal
  storage, aggregate copy/materialization, or a directly proven adjacent owner.

Completion check: a focused failing test or trace demonstrates the missing
semantic rule and excludes the old prior-preservation fallback route.

### Step 3: Repair The Narrow Owner

- Update only the first proven semantic owner.
- Preserve existing aggregate initialization behavior outside the failing
  feature shape.
- Add or update focused coverage for the repaired rule.
- Avoid broad ABI, call-boundary, or aggregate pipeline rewrites unless Step 1
  and Step 2 prove that owner is directly responsible.

Completion check: the focused test passes, and the implementation does not
contain `00216`-specific or initializer-spelling-specific matching.

### Step 4: Validate The 00216 Family

- Run the delegated narrow proof for `c_testsuite_aarch64_backend_src_00216_c`.
- Run the focused aggregate initializer or compound literal coverage added or
  updated in Step 3.
- Run nearby same-feature tests needed to reject a named-test-only fix.
- Record exact proof commands and results in `todo.md`.

Completion check: `00216` and focused same-feature proof pass without weakened
contracts or unrelated expectation changes.
