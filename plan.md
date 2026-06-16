# Plan: C Aggregate Function-Pointer Call IR Type Repair

Status: Active
Source Idea: ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md

## Purpose

Repair C lowering for aggregate-valued expressions passed through function
pointer calls so call emission uses the callee ABI parameter type instead of
passing an aggregate value where LLVM expects a scalar argument.

## Goal

Make `llvm_gcc_c_torture_src_struct_ret_1_c` stop emitting invalid IR for the
aggregate/function-pointer boundary, without weakening the torture harness or
special-casing the testcase.

## Core Rule

Fix the semantic lowering path that adapts aggregate returns or aggregate
arguments to the function-pointer ABI signature before call emission.

## Read First

- `ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md`
- `tests/c/external/gcc_torture/src/struct-ret-1.c`
- Call lowering and C aggregate ABI/type adaptation code near the generated
  invalid call.

## Current Targets

- Failing test:
  `ctest --test-dir build_debug -R '^llvm_gcc_c_torture_src_struct_ret_1_c$' --output-on-failure`
- Invalid IR shape to remove:
  an aggregate `%struct._anon_3` value passed as an `i32` call argument.
- Nearby validation:
  aggregate return and function-pointer call tests that exercise the same ABI
  boundary.

## Non-Goals

- Do not weaken the gcc torture runtime harness.
- Do not classify `struct-ret-1.c` unsupported without explicit approval.
- Do not special-case `struct-ret-1.c` or its generated symbol names.
- Do not fold in C++ dependent cast work or AArch64 fp128/vararg crash work.
- Do not start a broad ABI rewrite unless the localized boundary proves that
  broader ownership is required.

## Working Model

The current failure indicates call emission sees the function pointer callee
signature as scalar ABI types, but the argument value still has aggregate IR
type. The repair should identify where the aggregate expression is materialized
and where function-pointer ABI parameter types are known, then ensure the value
is lowered or extracted through the same authority used for the callee ABI
signature.

## Execution Rules

- Prefer structured type/ABI metadata over textual type matching.
- Preserve direct aggregate return behavior while repairing the indirect call
  path.
- Treat expectation rewrites, testcase skips, and harness changes as blockers,
  not progress.
- Each code-changing step needs fresh build proof before narrow CTest proof.
- Escalate to a reviewer if the proposed route depends on named-case matching
  or broad ABI churn without focused evidence.

## Ordered Steps

### Step 1: Locate The Aggregate Function-Pointer Boundary

Goal: identify the first lowering point where the aggregate value and scalar
callee ABI parameter type diverge.

Primary targets:

- `tests/c/external/gcc_torture/src/struct-ret-1.c`
- C call/function-pointer lowering code
- aggregate return or argument ABI adaptation helpers

Actions:

- Reproduce the focused failure and capture the invalid IR.
- Trace the value feeding the bad `i32` argument and record whether it is an
  aggregate return, aggregate load, or argument-preparation artifact.
- Identify the function or helper that owns adapting actual argument values to
  function-pointer parameter ABI types.
- Record the observed boundary and candidate repair surface in `todo.md`.

Completion check:

- `todo.md` names the first bad boundary, the owning lowering helper, and the
  exact proof command used to reproduce the current failure.

### Step 2: Repair ABI Argument Adaptation For Function-Pointer Calls

Goal: make function-pointer call argument lowering adapt aggregate-valued
expressions to the callee ABI parameter type before LLVM call emission.

Primary targets:

- The helper identified in Step 1
- Existing aggregate return and call ABI adaptation utilities

Actions:

- Reuse existing ABI/type adaptation authority when possible.
- Convert or extract aggregate values only through semantic aggregate ABI rules.
- Keep direct aggregate return behavior unchanged.
- Avoid testcase-shaped symbol or filename checks.

Completion check:

- The failing call no longer passes an aggregate `%struct._anon_3` value where
  LLVM expects `i32`.
- `cmake --build build_debug` passes.
- The focused CTest passes or advances to a different documented non-IR-type
  failure.

### Step 3: Prove Nearby Aggregate And Function-Pointer Coverage

Goal: show the fix is not limited to `struct-ret-1.c` and did not regress
nearby aggregate call behavior.

Actions:

- Run the focused target:
  `ctest --test-dir build_debug -R '^llvm_gcc_c_torture_src_struct_ret_1_c$' --output-on-failure`
- Run a nearby subset covering C aggregate returns and function-pointer calls.
- Inspect representative IR for the repaired path and record the before/after
  shape in `todo.md`.

Completion check:

- Focused and nearby tests are green, or any remaining failure is documented as
  outside this source idea.
- `todo.md` records the exact commands and result summary.

### Step 4: Acceptance Review And Closure Readiness

Goal: prepare the active lifecycle state for supervisor validation and closure.

Actions:

- Verify no test expectations, harness behavior, or unsupported classification
  were weakened.
- Confirm the repair is semantic rather than named-case-specific.
- Record final proof and any residual risks in `todo.md`.

Completion check:

- Source idea acceptance criteria are satisfied.
- No reviewer reject signal from the source idea applies.
- The supervisor has enough proof to run the close-time regression guard.
