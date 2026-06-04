# AArch64 00204 Stdarg HFA Runtime Repair Runbook

Status: Active
Source Idea: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md

## Purpose

Repair the remaining AArch64 backend runtime mismatch in
`c_testsuite_aarch64_backend_src_00204_c` as a semantic ABI issue involving
stdarg, variadic handoff, and HFA payload behavior.

## Goal

Make `00204.c` pass on the AArch64 backend without regressing the recently
restored `00032.c` and `00182.c` cases, and without weakening supported-path
expectations.

## Core Rule

Identify and repair the underlying AArch64 call/variadic/HFA ABI fact. Do not
special-case `00204.c`, rewrite expected output, or mark the testcase
unsupported.

## Read First

- `ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- Relevant backend call, variadic, liveness, and prepared-frame contract tests
- The `00204.c`, `00032.c`, and `00182.c` AArch64 backend test routes

## Current Targets

- Reproduce and characterize the `00204.c` runtime mismatch under the AArch64
  backend.
- Separate stdarg string/integer corruption from HFA float, double, and long
  double payload corruption.
- Locate the first wrong fact across BIR call ABI, prealloc call plans,
  variadic entry plans, call moves, and AArch64 call lowering.
- Add focused contract coverage for the repaired ABI fact before broad
  c-testsuite expectation changes.
- Preserve green coverage for `00032.c`, `00182.c`, and relevant backend
  internal tests.

## Non-Goals

- Do not rewrite the whole AArch64 calls owner layout.
- Do not downgrade `00204.c` expectations or mark it unsupported.
- Do not perform broad refactors of dispatch, memory, or unrelated prepared
  authority code.
- Do not fix unrelated x86 or RISC-V behavior.
- Do not claim progress from expectation-only or classification-only changes.

## Working Model

Recent pointer-carrier and frame-address work restored the newly regressed
AArch64 cases `00032.c` and `00182.c`. The remaining targeted failure is a
pre-existing ABI-heavy `00204.c` runtime mismatch. Treat it as a stdarg,
variadic-entry, and HFA payload repair route rather than as a continuation of
pointer-carrier cleanup.

## Execution Rules

- Keep investigation notes, current failure signatures, and proof commands in
  `todo.md`.
- Prefer ABI-fact probes over named-testcase matching.
- Add or strengthen internal contract coverage before changing broad
  c-testsuite expectations.
- Preserve the existing supported-path contract for `00204.c`.
- For code-changing steps, run `cmake --build --preset default` before handing
  the packet back.
- Narrow proof must include `c_testsuite_aarch64_backend_src_00204_c`,
  `c_testsuite_aarch64_backend_src_00032_c`, and
  `c_testsuite_aarch64_backend_src_00182_c`.
- Include relevant call, variadic, HFA, prepared-frame, or liveness contract
  tests when the repaired fact touches those layers.

## Step 1: Reproduce And Characterize 00204

Goal: capture the current `00204.c` failure precisely enough to split stdarg
corruption from HFA payload corruption.

Primary targets:

- `c_testsuite_aarch64_backend_src_00204_c`
- The generated AArch64 backend route artifacts for `00204.c`
- Existing debug flags or dumps that expose BIR, prealloc, call moves, and
  AArch64 call lowering facts

Actions:

- Reproduce the failing `00204.c` runtime behavior under the AArch64 backend.
- Record the observed wrong output and identify which argument groups are
  wrong.
- Compare string/integer stdarg behavior against float, double, and long
  double HFA payload behavior.
- Run `00032.c` and `00182.c` as guard cases if the reproduction command
  touches shared AArch64 backend state.
- Record the exact commands, outputs, and initial failure split in `todo.md`.

Completion check:

- `todo.md` names the current `00204.c` failure signature, the corrupted ABI
  payload class or classes, and the narrow reproduction command.
- Guard status for `00032.c` and `00182.c` is recorded or explicitly deferred
  with a reason.

## Step 2: Trace The First Wrong ABI Fact

Goal: locate the layer where the first incorrect stdarg or HFA fact appears.

Primary targets:

- BIR call ABI facts feeding prealloc
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/variadic_entry_plans.cpp`
- `src/backend/prealloc/regalloc/call_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`

Actions:

- Trace how `00204.c` arguments are classified and handed off through BIR,
  prealloc call planning, variadic entry planning, call moves, and AArch64
  lowering.
- Identify whether the first wrong fact is a call-site argument placement, a
  variadic save-area or entry-layout fact, a move/regalloc fact, or final
  machine lowering.
- Check nearby stdarg/HFA cases if needed to avoid fitting only `00204.c`.
- Record the first wrong fact and the last known-good upstream fact in
  `todo.md`.

Completion check:

- `todo.md` identifies one owned layer for repair and explains why earlier
  layers are not the source of the mismatch.
- The route does not depend on matching the literal `00204.c` shape.

## Step 3: Add Focused Contract Coverage

Goal: pin the repaired ABI fact with a focused internal test before changing
the implementation.

Primary targets:

- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prepare_liveness_test.cpp`
- Existing call, variadic, or HFA contract tests near the owned layer

Actions:

- Choose the smallest internal test surface that exposes the wrong ABI fact.
- Add or strengthen a contract test for the stdarg, variadic-entry, HFA, or
  call-move fact identified in Step 2.
- Avoid writing a test that only mirrors the full `00204.c` testcase output.
- Run the focused test and record the expected failing or passing status in
  `todo.md`.

Completion check:

- The new or strengthened contract would fail for the identified ABI bug or
  otherwise records the missing coverage gap that must be closed with the
  repair.
- `todo.md` names the test target and the exact ABI fact it proves.

## Step 4: Implement The Semantic ABI Repair

Goal: repair the owned stdarg, variadic, HFA, call-move, or AArch64 lowering
fact without broad backend rewrites.

Actions:

- Modify only the layer identified in Step 2 plus narrow helper/test support.
- Preserve the existing behavior for restored AArch64 cases `00032.c` and
  `00182.c`.
- Keep special-case compatibility code contained; do not add new named-case or
  literal-output shortcuts.
- Update nearby comments only when they explain a non-obvious ABI rule.

Completion check:

- `cmake --build --preset default` passes.
- The focused contract test from Step 3 passes.
- The diff is scoped to the owned ABI fact and required tests.

## Step 5: Prove The AArch64 Targeted Cases

Goal: prove the repaired route on the targeted c-testsuite cases and internal
guards.

Actions:

- Run the narrow c-testsuite proof including:
  - `c_testsuite_aarch64_backend_src_00204_c`
  - `c_testsuite_aarch64_backend_src_00032_c`
  - `c_testsuite_aarch64_backend_src_00182_c`
- Run the relevant internal call, variadic, HFA, prepared-frame, or liveness
  contract tests.
- If `00204.c` is decomposed into smaller probes, record each probe's ABI fact
  and proof result in `todo.md`.
- Investigate any regression before widening scope.

Completion check:

- `00204.c`, `00032.c`, and `00182.c` all pass under the delegated proof
  command.
- Internal contract coverage for the repaired fact passes.
- No supported-path expectation has been downgraded.

## Step 6: Final Validation And Close Readiness

Goal: prepare the repaired source idea for supervisor review and later closure.

Actions:

- Run the final delegated build and test subset chosen by the supervisor.
- Escalate to broader backend validation if shared call, variadic, regalloc, or
  AArch64 lowering interfaces changed.
- Update `todo.md` with final proof, residual risk, and close-readiness notes.

Completion check:

- Fresh proof covers the targeted AArch64 c-testsuite cases and the focused
  internal contract tests.
- `todo.md` records whether broader validation is recommended before closure.
- The route satisfies the source idea's reviewer reject signals.
