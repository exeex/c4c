# AArch64 Aggregate HFA Stdarg ABI Runbook

Status: Active
Source Idea: ideas/open/15_aarch64_aggregate_hfa_stdarg_abi.md

## Purpose

Repair the AArch64 aggregate, HFA, and stdarg ABI failure represented by
`c_testsuite_aarch64_backend_src_00204_c`.

## Goal

Identify and fix the semantic ABI classification, variadic access plan, or
lowering rule that causes the `00204` runtime mismatch.

## Core Rule

Repair AAPCS64 aggregate, HFA, or stdarg lowering semantics. Do not
special-case `00204`, one aggregate spelling, one callee name, or c_testsuite
expectations.

## Read First

- `ideas/open/15_aarch64_aggregate_hfa_stdarg_abi.md`
- `tests/c/external/c-testsuite/src/00204.c`
- Existing AArch64 aggregate, HFA, and variadic argument classification code.
- Prepared call, argument access-plan, and stdarg lowering paths implicated by
  reproduction.
- Focused backend tests covering aggregate, HFA, and variadic call behavior.

## Current Targets

- `c_testsuite_aarch64_backend_src_00204_c`
- The first proven AArch64 aggregate/HFA/stdarg ABI owner.
- Focused coverage for the repaired classification, access-plan, or lowering
  rule.

## Non-Goals

- Do not reopen prior-preservation selected fallback behavior.
- Do not repair generic aggregate initializer layout for `00216`.
- Do not fold recursion or global-array runtime work for `00181` into this
  route.
- Do not rewrite the full call ABI pipeline unless tracing proves the failing
  rule cannot be repaired locally.
- Do not weaken c_testsuite expectations or mark the target unsupported.

## Working Model

`00204` remained red after the source-selection family was repaired and was
classified as a distinct AArch64 aggregate/HFA/stdarg ABI family. Treat the
likely fault domain as ABI classification, variadic aggregate access, HFA
handling, or lowering from prepared facts into call/va_arg materialization
until reproduction proves a narrower owner.

## Execution Rules

- Start from a fresh reproduction of
  `c_testsuite_aarch64_backend_src_00204_c`.
- Trace from the observed runtime mismatch to the first incorrect ABI fact:
  classification, register/stack placement, variadic save-area access, HFA
  lane handling, aggregate copy, or materialization.
- Prefer semantic AAPCS64 checks and focused tests over named c_testsuite
  matching.
- Keep proof and packet progress in `todo.md`; do not edit the source idea for
  routine execution notes.
- Escalate to reviewer or plan-owner if evidence shows this is not an
  aggregate/HFA/stdarg ABI issue.

## Steps

### Step 1: Reproduce And Localize The 00204 ABI Failure

- Run the supervisor-selected narrow proof for
  `c_testsuite_aarch64_backend_src_00204_c`.
- Inspect the failing program behavior and identify the first bad observable:
  argument value, HFA element, aggregate field, variadic retrieval, stack slot,
  register assignment, or materialized address/value.
- Use dumps or targeted instrumentation only enough to locate the earliest bad
  classification or lowering owner.

Completion check: the failure is reproducible, and the suspected owner is
narrowed to a concrete AArch64 ABI classification, access-plan, stdarg, or
lowering path.

### Step 2: Prove The Missing ABI Rule

- Build a reduced focused test or trace for the same aggregate/HFA/stdarg rule
  without making `00204` the implementation condition.
- Compare nearby same-feature cases: non-variadic aggregate passing, HFA
  argument passing, variadic aggregate access, and any existing related
  backend tests.
- Document in `todo.md` why the issue is ABI classification, variadic
  access-plan construction, HFA lowering, or a directly proven adjacent owner.

Completion check: a focused failing test or trace demonstrates the missing ABI
rule and excludes prior-preservation fallback selection as the fix route.

### Step 3: Repair The Narrow ABI Owner

- Update only the first proven semantic owner.
- Preserve existing prepared call source authority and argument preservation
  behavior outside the repaired ABI rule.
- Add or update focused AArch64 backend coverage for the repaired
  aggregate/HFA/stdarg behavior.
- Avoid broad call-pipeline rewrites unless Step 1 and Step 2 prove that the
  localized ABI owner cannot express the required rule.

Completion check: the focused test passes, and the implementation contains no
`00204`-specific, function-name-specific, or literal-shape shortcut.

### Step 4: Validate The Aggregate/HFA/Stdarg Family

- Run the delegated narrow proof for `c_testsuite_aarch64_backend_src_00204_c`.
- Run the focused aggregate/HFA/stdarg coverage added or updated in Step 3.
- Run nearby existing variadic aggregate and HFA backend tests needed to reject
  a named-test-only fix.
- Record exact proof commands and results in `todo.md`.

Completion check: `00204`, focused same-feature proof, and existing nearby
aggregate/HFA/stdarg tests pass without weakened contracts or unsupported
markings.
