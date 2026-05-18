# AArch64 C-Testsuite Backend Runtime Execution Runbook

Status: Active
Source Idea: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md

## Purpose

Add an acceptance-quality AArch64 backend c-testsuite runtime route that emits
backend `.s`, asks clang to build an executable from that assembly, runs the
binary through a native host or configured runner, and compares output against
the existing c-testsuite expected files.

This is a route and proof plan first. It must not become a named-case lowering
shortcut campaign.

## Read First

- `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`
- `tests/c/external/CMakeLists.txt`
- `tests/c/external/c-testsuite/RunCase.cmake`
- `tests/c/external/c-testsuite-aarch64-backend-runner.cmake`
- Existing frontend c-testsuite LLVM IR flow
- Existing AArch64 backend smoke tests under `tests/backend/`

## Current Scope

- Make the AArch64 backend c-testsuite route explicit and runnable.
- Preserve stage-specific diagnostics.
- Prove focused cases through `.s -> clang -> runtime`.
- Preserve a broader failure summary for follow-up planning.

## Non-Goals

- Do not make the full c-testsuite pass in one plan.
- Do not weaken expected outputs, allowlists, or runtime checks.
- Do not patch named c-testsuite files in target lowering.
- Do not route through LLVM IR when proving backend execution.
- Do not change x86 backend behavior.

## Ordered Steps

### Step 1: Audit Current Runner And Registration

Goal: identify exactly what already works and what is missing for AArch64
backend runtime execution.

Actions:
- Inspect the frontend c-testsuite flow and current backend branches in
  `RunCase.cmake`.
- Inspect the AArch64-specific backend runner.
- Confirm how tests are registered when
  `ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON`.
- Identify whether the missing work is CMake registration, runner contract,
  proof target, diagnostics, or backend capability.
- Record the first implementation packet and proof command in `todo.md`.

Completion check:
- The next code packet can name the exact files to edit and the focused proof.

### Step 2: Normalize The AArch64 Runtime Runner Contract

Goal: make the non-native runtime path explicit and reliable.

Actions:
- Document or enforce how `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is invoked.
- Ensure runner command parsing handles a command plus arguments and appends the
  linked binary path.
- Preserve direct execution on native AArch64 hosts.
- Preserve `[RUNTIME_UNAVAILABLE]` when no runner exists on non-AArch64 hosts.

Completion check:
- A reviewer can configure or intentionally omit the runner and predict the
  failure class.

### Step 3: Add Focused Backend Runtime Proof

Goal: create a small proof route over representative c-testsuite cases.

Actions:
- Register or document a focused CTest subset for simple c-testsuite files such
  as `src/00001.c`, `src/00002.c`, and `src/00003.c`.
- Prove that these cases use `--codegen asm --target aarch64-unknown-linux-gnu`
  and produce `.s`, not LLVM IR.
- Build with clang from assembler input and run through native/runner runtime.
- Compare output to `.expected`.

Completion check:
- At least representative cases prove the full backend runtime path when the
  host/runner supports execution, or fail specifically as runtime-unavailable.

### Step 4: Run Broader AArch64 Backend C-Testsuite Route

Goal: run the registered AArch64 backend c-testsuite route and classify the
remaining failures.

Actions:
- Configure/build a backend-enabled scan tree if needed.
- Run the AArch64 backend c-testsuite CTest label/regex.
- Summarize counts by stage:
  frontend/BIR, backend `.s`, fallback rejection, assembler/link, runtime
  unavailable, runtime nonzero, runtime mismatch, pass.
- Preserve representative cases for each nontrivial failure family.

Completion check:
- The plan has actionable evidence for follow-up AArch64/BIR/runtime work.

### Step 5: Close Readiness

Goal: prove the route is ready to become a standing backend pressure source.

Actions:
- Run focused proof again after any runner/CMake edits.
- Run broader route enough to confirm the registration is not a one-case
  overfit.
- Confirm no expectations or allowlists were weakened.
- Confirm follow-up failures have clear owner layers.

Completion check:
- The supervisor can request plan-owner closure review with route proof and
  failure inventory.
