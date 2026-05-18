# AArch64 C-Testsuite Backend Runtime Execution Plan

Status: Active
Source Idea: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Reactivated After: ideas/closed/282_aarch64_loop_branch_control_runtime_hang.md

## Purpose

Return to the AArch64 c-testsuite backend runtime route after the focused
backend/codegen follow-up chain repaired the first runtime blockers through
`00006.c`.

## Goal

Decide whether the AArch64 backend c-testsuite runtime execution route is now
complete under idea 276, or produce a truthful broader failure inventory and
split any remaining backend/codegen/BIR owner layers into focused follow-up
ideas.

## Core Rule

Prove the real AArch64 backend assembly route:
`c4cll --codegen asm --target aarch64-unknown-linux-gnu -> clang -x assembler
-> executable -> runtime -> expected-output`. Do not count LLVM IR fallback,
runtime-unavailable, expectation weakening, or testcase-shaped backend fixes as
route completion.

## Read First

- `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`
- `todo.md`
- `ideas/closed/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md`
- `ideas/closed/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md`
- `ideas/closed/282_aarch64_loop_branch_control_runtime_hang.md`
- `ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md`
- AArch64 backend c-testsuite CMake runner files and latest
  `test_after.log`, if present, for focused route evidence

## Current Targets

- Confirm the runtime route remains configured and proves backend `.s` output,
  clang assembler/link, executable run, and expected-output comparison.
- Preserve the green focused chain: `00001.c` through `00006.c`.
- Run a broader registered AArch64 backend c-testsuite scan and group failures
  by route stage.
- Split any remaining backend/codegen/BIR capability gaps into focused source
  ideas rather than folding them into this route-readiness plan.

## Non-Goals

- Do not make the full c-testsuite corpus pass in one idea.
- Do not weaken `.expected` files, allowlists, unsupported classifications,
  CTest expectations, runner files, or timeout policy.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not hide missing semantic BIR facts inside AArch64 codegen.
- Do not add filename-specific lowering or printer hacks.
- Do not rework the x86 backend route.

## Working Model

Idea 276 registered and validated the AArch64 backend runtime route, then
split the first backend/codegen failures into focused ideas. Those follow-ups
are now closed through idea 282, and `00001.c` through `00006.c` pass through
the AArch64 backend runtime route.

The next lifecycle step is not to claim broad backend completeness. It is to
re-run the route-readiness review, confirm the representative runtime path is
still real backend assembly, and gather a current broader inventory for the
next owner layer.

## Execution Rules

- Keep route proof separate from backend semantic repairs.
- Update `todo.md` with exact proof commands, stage counts, and any follow-up
  owner layer.
- Preserve `test_after.log` as the canonical executor proof log for routine
  packets unless the supervisor delegates another artifact.
- Treat runtime-unavailable, runtime mismatch, nonzero exit, backend failure,
  assembler/link failure, and frontend/BIR failure as distinct route stages.
- Split backend/codegen/BIR repair work into focused follow-up ideas instead
  of expanding this route-readiness plan.

## Steps

### Step 1: Reconfirm Focused Runtime Route

Goal: prove the representative AArch64 backend runtime path is still the real
backend assembly route after the focused repairs.

Actions:

- Run the focused backend route for `00001.c` through `00006.c`.
- Confirm the path emits AArch64 assembly, assembles/links with clang using
  `--target=aarch64-unknown-linux-gnu -x assembler`, runs through the native or
  configured runner path, and compares expected output.
- Confirm no LLVM IR fallback, expectation weakening, or unsupported
  classification change is involved.
- Record exact proof in `todo.md`.

Completion check:

- `todo.md` states whether `00001.c` through `00006.c` pass the backend
  `.s -> clang -> executable -> runtime -> expected-output` route and names any
  route-stage blocker.

### Step 2: Broader AArch64 Backend Runtime Inventory

Goal: produce a current broader failure inventory for the registered AArch64
backend c-testsuite route.

Actions:

- Run the broader registered AArch64 backend c-testsuite scan.
- Group selected cases by route stage, including runtime unavailable, frontend
  failure, backend failure, assembler/link failure, runtime nonzero, runtime
  mismatch, timeout/hang, and pass.
- Preserve representative failing cases and their first owner-layer evidence.
- Do not repair backend semantic failures inside this route-inventory packet.

Completion check:

- `todo.md` records selected count, pass count, failure-stage counts, and the
  first distinct owner layer that should become a follow-up idea if any.

### Step 3: Completion Or Split Review

Goal: decide whether idea 276 can close or must split the next owner layer.

Actions:

- Compare route evidence against
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`.
- If the route infrastructure criteria are satisfied and remaining failures
  are backend capability gaps, create focused follow-up idea(s) and request
  closure review for idea 276.
- If route infrastructure is still incomplete, keep the blocker in `todo.md`
  without expanding into backend repairs.
- If no meaningful blocker remains under the route-readiness scope, request
  plan-owner closure review.

Completion check:

- `todo.md` clearly says whether idea 276 is ready for closure review or names
  the exact route blocker/follow-up owner layer.
