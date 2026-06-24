# Frontend LLVM Indirect Function Pointer Return Type Regression Runbook

Status: Active
Source Idea: ideas/open/327_frontend_llvm_indirect_function_pointer_return_type_regression.md

## Purpose

Repair the frontend LLVM-route lowering for indirect calls whose callee returns
a function pointer.

Goal: make `c_testsuite_src_00124_c` and neighboring function-pointer-return
indirect call shapes preserve the returned pointer type instead of collapsing
the first indirect call result to `i32`.

## Core Rule

Keep the fix semantic and type-driven. Do not special-case `00124.c`, `f1`,
`f2`, `p`, or rendered LLVM text that only matches the known failing case.

## Read First

- `ideas/open/327_frontend_llvm_indirect_function_pointer_return_type_regression.md`
- The frontend call-lowering path for direct and indirect calls.
- Existing frontend/LIR or LLVM-route tests that inspect function-pointer calls.

## Current Scope

- Reproduce `tests/c/external/c-testsuite/src/00124.c` through the frontend
  LLVM route.
- Locate where indirect-call result typing loses a function-pointer return
  type.
- Repair result type propagation for indirect calls returning function pointers.
- Add focused regression coverage for a function pointer returning another
  function pointer, including a chained indirect call.
- Prove the focused c-testsuite case and a neighboring frontend/LIR subset.

## Non-Goals

- Do not work on `backend_riscv_prepared_edge_publication`.
- Do not broaden into variadic ABI work, RV64 prepared variadic support, or
  external `printf` runtime support.
- Do not rewrite all function-pointer lowering unless evidence shows the shared
  type path is the defect.
- Do not weaken `c_testsuite_src_00124_c`, mark it unsupported, or reroute it
  away from the failing LLVM verification path.
- Do not use a broad c-testsuite sweep as the only proof for this small fix.

## Working Model

The failing IR already declares `f1` as returning `ptr`, but the indirect call
through the loaded local function pointer is emitted as `call i32 (...)`.
The first indirect call result must remain a pointer that can be invoked as a
function pointer. The repair should happen at the type-propagation point that
selects the LLVM function type and call result type for indirect callees.

## Execution Rules

- Capture evidence before editing the lowering path.
- Add focused coverage before or with the fix.
- Prefer existing type APIs and call-lowering helpers over ad hoc IR string
  checks.
- Keep proof logs at the repo root canonical: `test_before.log` and
  `test_after.log` only.
- Treat any test expectation downgrade or named-case shortcut as route failure.

## Step 1: Reproduce And Locate The Type Loss

Goal: establish the current failing IR shape and identify the lowering point
that chooses the wrong result type.

Primary targets:

- `tests/c/external/c-testsuite/src/00124.c`
- Frontend LLVM-route call lowering for direct and indirect calls.

Actions:

- Run the focused c-testsuite test and capture the current failure.
- Generate or inspect the LLVM IR for `00124.c` under `build/`.
- Trace how the indirect callee type is represented before LLVM call emission.
- Compare direct-call and indirect-call handling at the point that builds the
  LLVM function type.

Completion check:

- The responsible lowering/type-propagation site is identified, and the current
  wrong `call i32` result for the first indirect call is reproducible.

## Step 2: Add Focused Regression Coverage

Goal: make the nested indirect function-pointer return case fail for the right
reason before or alongside the implementation fix.

Primary targets:

- Existing frontend/LIR or LLVM-route regression tests for function-pointer
  calls.

Actions:

- Add a narrow test that covers a function pointer returning another function
  pointer.
- Include a chained indirect invocation equivalent to
  `(*(*p)(0, 2))(2, 2)`.
- Assert the first indirect call result is pointer-typed and does not produce a
  `load i32, ptr %t1` mismatch.
- Avoid matching source-specific names as the proof of correctness.

Completion check:

- The new coverage fails on the pre-fix behavior or is introduced in the same
  commit with a fix that clearly exercises the repaired generic path.

## Step 3: Repair Indirect Call Result Typing

Goal: preserve function-pointer return types when lowering indirect calls to
LLVM IR.

Primary targets:

- The call-lowering/type conversion code identified in Step 1.

Actions:

- Use the callee's function type or semantic function-pointer target type to
  derive the LLVM call return type.
- Ensure indirect and direct calls follow the same return-type conversion rules
  where applicable.
- Keep the change limited to the shared typed lowering path; do not add
  testcase-shaped branches.
- Confirm the returned function pointer can be used as the callee of the second
  indirect call.

Completion check:

- Generated LLVM IR for the focused source no longer emits the first indirect
  call as returning `i32`; it preserves a pointer return usable by the chained
  call.

## Step 4: Prove The Focused Fix

Goal: verify the repaired path without masking neighboring regressions.

Actions:

- Build or compile the touched frontend/backend target as required by the
  local build system.
- Run:
  `ctest --test-dir build -R '^c_testsuite_src_00124_c$' --output-on-failure`
- Run the focused regression test added in Step 2.
- Run a narrow neighboring frontend/LIR or c-testsuite subset selected by the
  supervisor.

Completion check:

- The focused c-testsuite test passes.
- The new focused regression passes.
- The neighboring subset has no new failures.
- Any remaining full-suite issue is still the known
  `backend_riscv_prepared_edge_publication` baseline failure, not part of this
  route.

## Step 5: Acceptance Review

Goal: confirm the route completed the source idea without overfitting.

Actions:

- Inspect the diff for named-case special casing, expectation downgrades, or
  unsupported reroutes.
- Compare the proof logs against the source idea acceptance criteria.
- Decide whether a broader regression guard or full suite is warranted before
  closing the idea.

Completion check:

- The source idea acceptance criteria are satisfied, and the route is ready for
  lifecycle closure only if the regression guard supports it.
